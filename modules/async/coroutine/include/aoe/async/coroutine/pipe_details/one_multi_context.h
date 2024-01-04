//
// Created by yarten on 24-1-4.
//

#pragma once

#include <optional>
#include "./one_one_context.h"
#include "./sized_vector.h"


namespace aoe::async::coroutine::pipe_details
{
    template<class T>
    class OneMultiContext
    {
        struct RecvSide
        {
            // every receiver owns an one-to-one context
            OneOneContext<T> ooc;

            // the unique sender owns all sending awaiter
            std::optional<typename OneOneContext<T>::SendAwaiter> send_awaiter;
        };
    public:
        OneMultiContext(
            const std::shared_ptr<Pool> pool, const std::size_t buffer_size, const std::size_t receivers_count) :
            pool_(pool),
            send_id_creator_(1),
            recv_id_creator_(receivers_count),
            buffer_size_(buffer_size),
            recv_sides_(receivers_count)
        {
        }

        SendId newSend()
        {
            // TODO: 需要遍历所有的接收端，建立
            return send_id_creator_.next();
        }

        RecvId newRecv()
        {
            const RecvId id = recv_id_creator_.next();

            if (id.valid())
                recv_sides_[id.num()].construct(pool_.lock(), buffer_size_);

            return id;
        }

        void deleteSend(const SendId id)
        {

        }

    private:
        std::weak_ptr<Pool> pool_;

        SendId::Creator send_id_creator_;
        RecvId::Creator recv_id_creator_;

        const std::size_t buffer_size_ = 0;
        SizedVector<RecvSide> recv_sides_;
    };
}
