//
// Created by yarten on 24-1-4.
//

#pragma once

#include <optional>
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

    template<class T>
    class OneMultiContext
    {
        struct RecvSide
        {
            // every receiver owns an one-to-one context
            OneOneContext<T> ooc;

            // the unique sender owns all sending awaiter
            std::optional<typename OneOneContext<T>::SendAwaiter> send_awaiter;

            RecvSide(const std::shared_ptr<Pool> pool, const std::size_t buffer_size, const bool send_closed)
                : ooc(pool, buffer_size)
            {
                ooc.newRecv();

                if (const SendId id = ooc.newSend(); send_closed)
                    ooc.deleteSend(id);
            }
        };
    public:
        OneMultiContext(
            const std::shared_ptr<Pool> pool,
            const std::size_t buffer_size,
            const std::size_t receivers_count,
            const BroadcastType broadcast_type)
        :
            pool_(pool),
            send_id_creator_(1),
            recv_id_creator_(receivers_count),
            buffer_size_(buffer_size),
            recv_sides_(receivers_count),
            broadcast_type_(broadcast_type),
            serving_recv_ids_(receivers_count)
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

            for(std::size_t idx = 0, end_idx = recv_sides_.capacity(); idx < end_idx; ++idx)
            {
                if (typename SizedVector<RecvSide>::Element & i = recv_sides_[idx]; i.isUsing())
                    i->ooc.deleteSend(id);
            }
        }

        void deleteRecv(const RecvId id)
        {
            if (not id.valid())
                return;

            recv_sides_[id.num()]->ooc.deleteRecv(id);
        }

    private:
        std::weak_ptr<Pool> pool_;

        SendId::Creator send_id_creator_;
        RecvId::Creator recv_id_creator_;

        const std::size_t buffer_size_ = 0;
        SizedVector<RecvSide> recv_sides_;

        // Defines how the only sender handles multiple receivers
        const BroadcastType broadcast_type_ = BroadcastType::None;

        // If the only sender is closed, all future newly created receivers will close their respective senders.
        std::atomic<bool> send_closed_ = false;

        // if no receiver has been created yet, the sender's coroutine is suspended, and its coroutine handle
        // with the send data is temporarily recorded here, to be taken by the first receiver when it receives it
        // and wakes up the sender.
        std::atomic<std::coroutine_handle<Base>> awaiting_send_;
        std::variant<std::nullptr_t, T, std::reference_wrapper<const T>> unique_send_data_;

        // The set of receiver ids that the sender needs to take care of in a single send.
        SizedStack<RecvId> serving_recv_ids_;

    public:
        class SendAwaiter : public BoolAwaiter<SendAwaiter>
        {
            using Super = BoolAwaiter<SendAwaiter>;
        public:

        private:
            friend class OneMultiContext;

            SendAwaiter(OneMultiContext & self, const T & data)
                requires(std::is_copy_assignable_v<T>)
                : Super(currentHandle()), self_(self)
            {

            }

            SendAwaiter(OneMultiContext & self, T && data)
                : Super(currentHandle()), self_(self)
            {

            }

        private:
            template<class E>
            void prepare(E && data)
            {

            }

        private:
            OneMultiContext & self_;
        };
    };
}
