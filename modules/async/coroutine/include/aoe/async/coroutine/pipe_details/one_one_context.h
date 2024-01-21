//
// Created by yarten on 24-1-1.
//

#pragma once

#include <variant>
#include <moodycamel/readerwriterqueue.h>

#include <aoe/trait.h>
#include "./id.h"
#include "../base.h"


namespace aoe::async::coroutine::pipe_details
{
    template<class T>
    class OneOneContext
    {
    public:
        OneOneContext(std::shared_ptr<Pool> pool, const std::size_t buffer_size) :
            pool_(std::move(pool)),
            send_id_creator_(1),
            recv_id_creator_(1),
            buffer_size_(buffer_size),
            buffer_(std::max(1UL, buffer_size))
        {
        }

        SendId newSend()
        {
            return send_id_creator_.next();
        }

        RecvId newRecv()
        {
            return recv_id_creator_.next();
        }

        void deleteSend(const SendId id)
        {
            if (not id.valid() or send_closed_.load(std::memory_order::relaxed))
                return;

            send_closed_.store(true, std::memory_order::release);
            awake(pool_, awaiting_recv_.exchange({}, std::memory_order::acquire));
        }

        void deleteRecv(const RecvId id)
        {
            if (not id.valid() or recv_closed_.load(std::memory_order::relaxed))
                return;

            recv_closed_.store(true, std::memory_order::release);
            awake(pool_, awaiting_send_.exchange({}, std::memory_order::acquire));
        }

        [[nodiscard]] bool isSendClosed() const
        {
            return send_closed_.load(std::memory_order::acquire);
        }

        [[nodiscard]] bool isRecvClosed() const
        {
            return recv_closed_.load(std::memory_order::acquire);
        }

    private:
        std::weak_ptr<Pool> pool_;

        SendId::Creator send_id_creator_;
        RecvId::Creator recv_id_creator_;

        std::atomic_bool send_closed_ = false;
        std::atomic_bool recv_closed_ = false;

        const std::size_t buffer_size_ = 0;
        moodycamel::ReaderWriterQueue<T> buffer_;

        std::atomic<std::coroutine_handle<Base>> awaiting_send_;
        std::atomic<std::coroutine_handle<Base>> awaiting_recv_;

        // This flag is used when the buffer size is zero.
        // It will be set to true when the receiver takes the data from the buffer,
        // and to false when the sender is suspended.
        std::atomic_bool send_is_awaked_once_ = false;

    public:
        class Sender
        {
        public:
            explicit Sender(OneOneContext & self)
                : self_(self)
            {
            }

            [[nodiscard]] bool isReady() const
            {
                if (self_.recv_closed_.load(std::memory_order::acquire))
                    return true;

                if (self_.buffer_size_ == 0)
                    return self_.awaiting_recv_.load(std::memory_order::acquire).address() != nullptr;

                // TODO: 被别人唤醒时，可能会调用这个函数进行检查，是否真的 ready，此时如果 buffer size 为零，那么 awaiting_recv
                // 一定不存在，且数据也已经被拿走。也许得补充检查一下？包括 OMC 的分发模式

                return self_.buffer_.size_approx() < self_.buffer_size_;
            }

            void send(const T & data) requires(std::is_copy_assignable_v<T>)
            {
                if (not self_.buffer_.enqueue(data))
                    throw std::bad_alloc();
            }

            void send(T && data)
            {
                if (not self_.buffer_.enqueue(std::move(data)))
                    throw std::bad_alloc();
            }

            template<class TMovedOrCopied>
            bool sendAndAwake(TMovedOrCopied && data)
            {
                if (self_.recv_closed_.load(std::memory_order::acquire))
                    return false;

                send(std::forward<TMovedOrCopied>(data));
                awake(self_.pool_, self_.awaiting_recv_.exchange({}, std::memory_order::acquire));
                return true;
            }

        private:
            OneOneContext & self_;
        };

        class SendAwaiter : public BoolAwaiter<SendAwaiter>
        {
            using Super = BoolAwaiter<SendAwaiter>;
        public:
            [[nodiscard]] bool isReady() const
            {
                return sender_.isReady();
            }

            void onSuspend(const std::coroutine_handle<Base> handle)
            {
                self_.awaiting_send_.store(handle, std::memory_order::release);

                // when the buffer size is zero, it is the case that sender and receiver exchange data directly.
                // when the sender is suspended to wait for the receiver, we should push the data to avoid that
                // the receiver is also suspended because of the empty buffer.
                if (self_.buffer_size_ == 0)
                {
                    pushData();
                    has_sent_the_data_ = true;
                }
            }

            bool onResume()
            {
                self_.awaiting_send_.store({}, std::memory_order::release);

                if (self_.recv_closed_.load(std::memory_order::acquire))
                    return false;

                if (self_.buffer_size_ != 0 or not has_sent_the_data_)
                {
                    pushData();
                    awake(self_.pool_, self_.awaiting_recv_.exchange({}, std::memory_order::acquire));
                }

                return true;
            }

            void onAbort()
            {
                self_.awaiting_send_.store({}, std::memory_order::release);

                if (self_.buffer_size_ == 0)
                    self_.buffer_.pop();
            }

        private:
            friend class OneOneContext;

            SendAwaiter(OneOneContext & self, const T & data)
                requires(std::is_copy_assignable_v<T>)
                : Super(currentHandle()), self_(self), data_(std::ref(data)), sender_(self)
            {
            }

            SendAwaiter(OneOneContext & self, T && data)
                : Super(currentHandle()), self_(self), data_(std::move(data)), sender_(self)
            {
            }

        private:
            void pushData()
            {
                std::visit(
                    trait::impl {
                        [&](T & i)
                        {
                            return sender_.send(std::move(i));
                        },
                        [&](std::reference_wrapper<const T> & i)
                        {
                            if constexpr (std::is_copy_assignable_v<T>)
                                return sender_.send(i.get());
                            else
                                panic.wtf("");
                        }
                    }, data_
                );
            }

        private:
            OneOneContext & self_;
            std::variant<T, std::reference_wrapper<const T>> data_;
            Sender sender_;

            // If the buffer size is zero, the data is sent when the sender is suspended,
            // we should aovid to send it again when the sender is resumed.
            bool has_sent_the_data_ = false;
        };

        class RecvAwaiter : public BoolAwaiter<RecvAwaiter>
        {
            using Super = BoolAwaiter<RecvAwaiter>;
        public:
            [[nodiscard]]
            bool isReady() const
            {
                if (self_.send_closed_.load(std::memory_order::acquire))
                    return true;

                return self_.buffer_.size_approx() != 0;
            }

            void onSuspend(const std::coroutine_handle<Base> handle)
            {
                self_.awaiting_recv_.store(handle, std::memory_order::release);
            }

            bool onResume()
            {
                self_.awaiting_recv_.store({}, std::memory_order::release);

                if (self_.send_closed_.load(std::memory_order::acquire))
                    return self_.buffer_.try_dequeue(result_);

                const bool must_have_at_least_one_value = self_.buffer_.try_dequeue(result_);
                assert(must_have_at_least_one_value);

                awake(self_.pool_, self_.awaiting_send_.exchange({}, std::memory_order::acquire));
                return true;
            }

            void onAbort()
            {
                self_.awaiting_recv_.store({}, std::memory_order::release);
            }

        private:
            friend class OneOneContext;

            RecvAwaiter(OneOneContext & self, T & result)
                : Super(currentHandle()), self_(self), result_(result)
            {
            }

        private:
            OneOneContext & self_;
            T & result_;
        };

        using SendAwaiterType = SendAwaiter;
        using RecvAwaiterType = RecvAwaiter;

    public:
        template<class TMovedOrCopied>
        SendAwaiterType send(const SendId id, TMovedOrCopied && data)
        {
            assert(id.valid());
            return {*this, std::forward<TMovedOrCopied>(data)};
        }

        RecvAwaiterType recv(const RecvId id, T & result)
        {
            assert(id.valid());
            return {*this, result};
        }
    };
}
