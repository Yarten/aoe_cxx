//
// Created by yarten on 24-1-1.
//

#pragma once

#include <variant>
#include <moodycamel/readerwriterqueue.h>
#include "./id.h"

#include "../base.h"


namespace aoe::async::coroutine::pipe_details
{
    template<class T>
    class OneOneContext
    {
    public:
        OneOneContext(const std::shared_ptr<Pool> pool, const std::size_t buffer_size) :
            pool_(pool),
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

    public:
        class SendAwaiter : public BoolAwaiter<SendAwaiter>
        {
            using Super = BoolAwaiter<SendAwaiter>;
        public:
            [[nodiscard]]
            bool isReady() const
            {
                if (self_.recv_closed_.load(std::memory_order::acquire))
                    return true;

                if (self_.buffer_size_ == 0)
                    return self_.awaiting_recv_.load(std::memory_order::acquire).address() != nullptr;

                return self_.buffer_.size_approx() < self_.buffer_size_;
            }

            void onSuspend(const std::coroutine_handle<Base> handle)
            {
                // when the buffer size is zero, it is the case that sender and receiver exchange data directly.
                // when the sender is suspended to wait for the receiver, we should push the data to avoid that
                // the receiver is also suspended because of the empty buffer.
                if (self_.buffer_size_ == 0 and self_.buffer_.size_approx() == 0)
                    pushData();

                self_.awaiting_send_.store(handle, std::memory_order::release);
            }

            bool onResume()
            {
                self_.awaiting_send_.store({}, std::memory_order::release);

                if (self_.recv_closed_.load(std::memory_order::acquire))
                    return false;

                if (self_.buffer_size_ != 0 or self_.buffer_.size_approx() == 0)
                    pushData();

                awake(self_.pool_, self_.awaiting_recv_.exchange({}, std::memory_order::acquire));
                return true;
            }

            void onAbort()
            {
                self_.awaiting_send_.store({}, std::memory_order::release);
            }

        private:
            friend class OneOneContext;

            SendAwaiter(OneOneContext & self, const T & data)
                : Super(currentHandle()), self_(self), data_(std::ref(data))
            {
            }

            SendAwaiter(OneOneContext & self, T && data)
                : Super(currentHandle()), self_(self), data_(std::move(data))
            {
            }

        private:
            void pushData()
            {
                bool status = false;

                constexpr std::size_t INDEX_VALUE = 0;
                constexpr std::size_t INDEX_REF   = 1;
                static_assert(std::is_same_v<decltype(std::get<INDEX_VALUE>(data_)), T>);
                static_assert(std::is_same_v<decltype(std::get<INDEX_REF>(data_)), const T &>);

                switch (data_.index())
                {
                case INDEX_VALUE:
                    status = self_.buffer_.enqueue(std::move(std::get<INDEX_VALUE>(data_)));
                    break;
                case INDEX_REF:
                    status = self_.buffer_.enqueue(std::get<INDEX_REF>(data_));
                default:
                    break;
                }

                if (not status)
                    throw std::bad_alloc();
            }

        private:
            OneOneContext & self_;
            std::variant<T, std::reference_wrapper<const T>> data_;
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

    public:
        SendAwaiter send(const SendId id, const T & data)
        {
            assert(id.valid());
            return {*this, data};
        }

        SendAwaiter send(const SendId id, T && data)
        {
            assert(id.valid());
            return {*this, std::move(data)};
        }

        RecvAwaiter recv(const RecvId id, T & result)
        {
            assert(id.valid());
            return {*this, result};
        }
    };
}
