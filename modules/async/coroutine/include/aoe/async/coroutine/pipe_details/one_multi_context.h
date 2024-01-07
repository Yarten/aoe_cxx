//
// Created by yarten on 24-1-4.
//

#pragma once

#include <optional>
#include <ranges>

#include "./atomic_optional.h"
#include "./one_one_context.h"
#include "./sized_vector.h"
#include "./sized_stack.h"


namespace aoe::async::coroutine::pipe_details
{
    enum class BroadcastType
    {
        None,
        Some,
        All
    };

    template <class T, BroadcastType BC_TYPE>
    requires(std::is_copy_assignable_v<T> or BC_TYPE == BroadcastType::None)
    class OneMultiContext
    {
        struct RecvSideBase
        {
            // Every receiver owns an one-to-one context.
            OneOneContext<T> ooc;

            RecvSideBase(const std::shared_ptr<Pool> pool, const std::size_t buffer_size, const bool send_closed)
                : ooc(pool, buffer_size)
            {
                ooc.newRecv();

                if (const SendId id = ooc.newSend(); send_closed)
                    ooc.deleteSend(id);
            }
        };

        template<BroadcastType>
        struct RecvSide : RecvSideBase
        {
            // In broadcast mode, we reuse OOC's send awaiter to await and send data.
            std::optional<typename OneOneContext<T>::SendAwaiter> send_awaiter;

            using RecvSideBase::RecvSideBase;
        };

        template<BroadcastType THIS_BC_TYPE> requires(THIS_BC_TYPE == BroadcastType::None)
        struct RecvSide<THIS_BC_TYPE> : RecvSideBase
        {
            // In dispatch mode (data can only be received by one receiver at a time),
            // We record the coroutine handle in this OMC when sending is suspended,
            // and send using the sender when sending is resumed.
            typename OneOneContext<T>::Sender sender;

            RecvSide(const std::shared_ptr<Pool> pool, const std::size_t buffer_size, const bool send_closed)
                : RecvSideBase(pool, buffer_size, send_closed), sender(RecvSideBase::ooc)
            {
            }
        };

    public:
        OneMultiContext(
            std::shared_ptr<Pool> pool, const std::size_t buffer_size, const std::size_t receivers_count)
        :
            pool_(std::move(pool)),
            send_id_creator_(1),
            recv_id_creator_(receivers_count),
            buffer_size_(buffer_size),
            recv_sides_(receivers_count),
            send_ctx_(receivers_count)
        {
        }

        SendId newSend()
        {
            return send_id_creator_.next();
        }

        RecvId newRecv()
        {
            const RecvId id = recv_id_creator_.next();

            if (id.valid())
                recv_sides_[id.num()].construct(
                    pool_.lock(), buffer_size_, send_closed_.load(std::memory_order::acquire)
                );

            return id;
        }

        void deleteSend(const SendId id)
        {
            if (not id.valid() or send_closed_.load(std::memory_order::relaxed))
                return;

            send_closed_.store(true, std::memory_order::release);

            for(auto [_, recv] : viewUsingRecvSides())
                recv.ooc.deleteSend(id);
        }

        void deleteRecv(const RecvId id)
        {
            if (not id.valid())
                return;

            recv_sides_[id.num()]->ooc.deleteRecv(id);
        }

    private:
        auto viewUsingRecvSides()
        {
            return
                recv_sides_.viewUsing()
            |
                std::views::transform(
                    [](const std::pair<std::size_t, RecvSide<BC_TYPE> &> & src)
                        -> std::pair<RecvId, RecvSide<BC_TYPE> &>
                    {
                        return {RecvId::fromNum(static_cast<std::uint32_t>(src.first)), src.second};
                    }
                );
        }

        auto viewOpeningRecvSides()
        {
            return
                viewUsingRecvSides()
            |
                std::views::filter(
                    [](const std::pair<RecvId, RecvSide<BC_TYPE> &> & pair)
                    {
                        return not pair.second.ooc.isRecvClosed();
                    }
                );
        }

    private:
        std::weak_ptr<Pool> pool_;

        SendId::Creator send_id_creator_;
        RecvId::Creator recv_id_creator_;

        const std::size_t buffer_size_ = 0;
        SizedVector<RecvSide<BC_TYPE>> recv_sides_;

        // If the only sender is closed, all future newly created receivers will close their respective senders.
        std::atomic<bool> send_closed_ = false;

    private:
        template<BroadcastType>
        struct SendContext
        {
            // The set of receiver ids that the sender needs to take care of in a single send.
            SizedStack<RecvId> serving_recv_ids;

            explicit SendContext(const std::size_t receivers_count)
                : serving_recv_ids(receivers_count)
            {
            }
        };

        template<BroadcastType THIS_BC_TYPE> requires(THIS_BC_TYPE == BroadcastType::None)
        struct SendContext<THIS_BC_TYPE>
        {
            // If no receiver has been created yet, the sender's coroutine is suspended, and its coroutine handle
            // with the send data is temporarily recorded here, to be taken by the first receiver when it receives it
            // and wakes up the sender.
            std::atomic<std::coroutine_handle<Base>> awaiting_send;
            AtomicOptional<std::variant<T, std::reference_wrapper<const T>>> unique_send_data;

            explicit SendContext(const std::size_t) {}
        };

        SendContext<BC_TYPE> send_ctx_;

    public:
        template<BroadcastType THIS_BC_TYPE>
        class SendAwaiter : public BoolAwaiter<SendAwaiter<THIS_BC_TYPE>>
        {
            using Super = BoolAwaiter<SendAwaiter>;
        public:
            [[nodiscard]] bool isReady() const
            {
                for (typename OneOneContext<T>::SendAwaiter & awaiter : viewRecvSidesSendAwaiters())
                {
                    if (awaiter.isReady())
                    {
                        if constexpr (THIS_BC_TYPE == BroadcastType::Some)
                            return true;
                    }
                    else
                    {
                        if constexpr (THIS_BC_TYPE == BroadcastType::All)
                            return false;
                    }
                }

                return true;
            }

            void onSuspend(const std::coroutine_handle<Base> handle)
            {
                for(typename OneOneContext<T>::SendAwaiter & awaiter : viewRecvSidesSendAwaiters())
                    awaiter.onSuspend(handle);
            }

            bool onResume()
            {
                bool status = false;

                for(typename OneOneContext<T>::SendAwaiter & awaiter : viewRecvSidesSendAwaiters())
                {
                    if constexpr (THIS_BC_TYPE == BroadcastType::Some)
                    {
                        if (not awaiter.isReady())
                        {
                            awaiter.onAbort();
                            continue;
                        }
                    }

                    status or_eq awaiter.onResume();
                }

                return status;
            }

            void onAbort()
            {
                for (typename OneOneContext<T>::SendAwaiter & awaiter : viewRecvSidesSendAwaiters())
                    awaiter.onAbort();
            }

        private:
            friend class OneMultiContext;

            SendAwaiter(OneMultiContext & self, const T & data)
                : Super(currentHandle()), self_(self)
            {
                self_.send_ctx_.serving_recv_ids.clear();

                for (auto [id, recv] : self_.viewOpeningRecvSides())
                {
                    recv.send_awaiter.emplace(recv.ooc.send(id, data));
                    self_.send_ctx_.serving_recv_ids_.push(id);
                }
            }

        private:
            auto viewRecvSidesSendAwaiters()
            {
                return
                    self_.send_ctx_.serving_recv_ids_.view()
                |
                    std::views::transform(
                        [this](const RecvId & id) -> typename OneOneContext<T>::SendAwaiter &
                        {
                            return self_.recv_sides_[id.num()]->send_awaiter.value();
                        }
                    );
            }

        private:
            OneMultiContext & self_;
        };

        template<BroadcastType THIS_BC_TYPE> requires(THIS_BC_TYPE == BroadcastType::None)
        class SendAwaiter<THIS_BC_TYPE> : public BoolAwaiter<SendAwaiter<THIS_BC_TYPE>>
        {
            using Super = BoolAwaiter<SendAwaiter>;
        public:
            [[nodiscard]] bool isReady() const
            {
                for (typename OneOneContext<T>::Sender & sender : viewRecvSidesSenders())
                {
                    if (sender.isReady())
                        return true;
                }

                // TODO: 若已经被挂起，然后被唤醒，可能会调用本函数检查是否真的 ready，针对这种情况，也许得检查是否是唯一唤醒

                // The sender will hang and wait if there is still a possibility to create a new receiver.
                return not self_.recv_id_creator_.hasNext();
            }

            void onSuspend(const std::coroutine_handle<Base> handle)
            {
                self_.send_ctx_.awaiting_send.store(handle, std::memory_order::release);
                has_been_suspended_ = true;
            }

            bool onResume()
            {
                // If the sender was once suspended and is now resumed, then one of the receivers
                // must have woken it up and has taken the data.
                if (has_been_suspended_)
                    return true;

                // If receivers are already ready, we send the data immediately.
                self_.send_ctx_.awaiting_send.store({}, std::memory_order::release);

                const std::unique_ptr<std::variant<T, std::reference_wrapper<const T>>> data_ptr =
                    self_.send_ctx_.unique_send_data.takeAndReset();
                assert(data_ptr != nullptr);

                bool status = false;

                for (typename OneOneContext<T>::Sender & sender : viewRecvSidesSenders())
                {
                    status =
                        sender.isReady()
                    and
                        std::visit(
                            trait::impl {
                                [&sender](T & i)
                                {
                                    return sender.sendAndAwake(std::move(i));
                                },
                                [&sender](std::reference_wrapper<const T> & i)
                                {
                                    return sender.sendAndAwake(i.get());
                                }
                            }, *data_ptr
                        );

                    if (status)
                        break;
                }

                // If all receiver are closed, status will be false.
                // When a receiver is closed, it is ready but can not be sent.
                return status;
            }

            void onAbort()
            {
                self_.send_ctx_.awaiting_send.store({}, std::memory_order::release);
                self_.send_ctx_.unique_send_data.reset();
            }

        private:
            friend class OneMultiContext;

            SendAwaiter(OneMultiContext & self, const T & data)
                requires(std::is_copy_assignable_v<T>)
                : Super(currentHandle()), self_(self)
            {
                self_.send_ctx_.unique_send_data.emplace(std::ref(data));
            }

            SendAwaiter(OneMultiContext & self, T && data)
                : Super(currentHandle()), self_(self)
            {
                self_.send_ctx_.unique_send_data.emplace(std::move(data));
            }

        private:
            void viewRecvSidesSenders()
            {
                return
                    self_.viewOpeningRecvSides()
                |
                    std::views::transform(
                        [](const std::pair<RecvId, RecvSide<BC_TYPE> &> & pair) -> typename OneOneContext<T>::Sender
                        {
                            return pair.secod.sender;
                        }
                    );
            }

        private:
            OneMultiContext & self_;
            bool has_been_suspended_ = false;
        };

        template<BroadcastType THIS_BC_TYPE>
        class RecvAwaiter : public BoolAwaiter<RecvAwaiter<THIS_BC_TYPE>>
        {
            using Super = BoolAwaiter<RecvAwaiter>;
        public:
            [[nodiscard]] bool isReady() const
            {
                return recv_awaiter_.isReady();
            }

            void onSuspend(const std::coroutine_handle<Base> handle)
            {
                recv_awaiter_.onSuspend(handle);
            }

            bool onResume()
            {
                return recv_awaiter_.onResume();
            }

            void onAbort()
            {
                recv_awaiter_.onAbort();
            }

        private:
            friend class OneMultiContext;

            RecvAwaiter(OneMultiContext & self, const RecvId id, T & result)
                : Super({}), recv_awaiter_(self.recv_sides_[id.num()]->ooc.recv(id, result))
            {
            }

        private:
            typename OneOneContext<T>::RecvAwaiter recv_awaiter_;
        };

        template<BroadcastType THIS_BC_TYPE> requires(THIS_BC_TYPE == BroadcastType::None)
        class RecvAwaiter<THIS_BC_TYPE> : public BoolAwaiter<RecvAwaiter<THIS_BC_TYPE>>
        {
            using Super = BoolAwaiter<RecvAwaiter>;
        public:
            [[nodiscard]] bool isReady() const
            {
                return not recv_awaiter_.has_value() or recv_awaiter_->isReady();
            }

            void onSuspend(const std::coroutine_handle<Base> handle)
            {
                recv_awaiter_->onSuspend(handle);
            }

            bool onResume()
            {
                return not recv_awaiter_.has_value() or recv_awaiter_->onResume();
            }

            void onAbort()
            {
                recv_awaiter_->onAbort();
            }

        private:
            friend class OneMultiContext;

            RecvAwaiter(OneMultiContext & self, const RecvId id, T & result)
                : Super({}), self_(self)
            {
                // If the sending coroutine is suspended, and this receiver is the lucky one to take the data,
                // it sholud awake the sending coroutine and then leave.
                const std::coroutine_handle<Base> send_ch =
                    self_.send_ctx_.awaiting_send.exchange({}, std::memory_order::acquire);

                if (send_ch.address() != nullptr)
                {
                    const std::unique_ptr<std::variant<T, std::reference_wrapper<const T>>> data_ptr =
                        self_.send_ctx_.unique_send_data.takeAndReset();

                    // This data pointer can be null when the sender is suddenly aborted at this monment.
                    if (data_ptr != nullptr)
                    {
                        std::visit(
                            trait::impl {
                                [&result](T & i)
                                {
                                    result = std::move(i);
                                    return true;
                                },
                                [&result](std::reference_wrapper<const T> & i)
                                {
                                    result = i.get();
                                    return true;
                                }
                            }, *data_ptr
                        );

                        awake(self_.pool_, send_ch);
                        return;
                    }
                }

                // If this receiver failed to take the data from OMC, it should take the data from its awaiter.
                recv_awaiter_.emplace(self_.recv_sides_[id.num()]->ooc.recv(id, result));
            }

        private:
            OneMultiContext & self_;
            std::optional<typename OneOneContext<T>::RecvAwaiter> recv_awaiter_;
        };

        using SendAwaiterType = SendAwaiter<BC_TYPE>;
        using RecvAwaiterType = RecvAwaiter<BC_TYPE>;

    public:
        template<class TMovedOrCopied>
        SendAwaiterType send(const SendId id, TMovedOrCopied && data)
        {
            assert(id.valid());
            return {this, std::forward<TMovedOrCopied>(data)};
        }

        RecvAwaiterType recv(const RecvId id, T & result)
        {
            assert(id.num() < recv_sides_.capacity());
            return {*this, id, result};
        }
    };
}
