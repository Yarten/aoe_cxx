//
// Created by yarten on 23-12-31.
//

#pragma once

#include <memory>
#include "./pipe_details/context.h"


namespace aoe::async::coroutine
{
    struct BroadcastNone
    {
    };

    struct BroadcastSome
    {
    };

    struct BroadcastAll
    {
    };

    template <class T>
    class Pipe
    {
        class RecvFuncAwaiter : public BoolAwaiter<RecvFuncAwaiter>
        {
        public:
            template <class F>
            RecvFuncAwaiter(Pipe& self, F&& func)
                :
                BoolAwaiter<RecvFuncAwaiter>(nullptr),
                awaiter_(
                    self >> result_
                    & [this, func = std::forward<F>(func)]()
                    {
                        func(std::move(result_));
                    }
                )
            {
            }

        public:
            [[nodiscard]] bool isReady() const
            {
                return awaiter_.isReady();
            }

            void onSuspend(const std::coroutine_handle<Base> handle)
            {
                awaiter_.onSuspend(handle);
            }

            bool onResume()
            {
                return awaiter_.doBoolAwaiterResume();
            }

            void onAbort()
            {
                awaiter_.doBoolAwaiterAbort();
            }

        private:
            T result_{};
            typename pipe_details::Context<T>::RecvAwaiter awaiter_;
        };

    public:
        Pipe() = default;
        Pipe(const Pipe&) = delete;
        Pipe& operator=(const Pipe&) = delete;

        Pipe deviler()
        {
            Pipe result;
            result.ctx_ = ctx_;
            return result;
        }

        Pipe(Pipe&& other) noexcept
        {
            operator=(std::move(other));
        }

        Pipe& operator=(Pipe&& other) noexcept
        {
            if (this == &other)
                return *this;

            destoryContext();

            ctx_ = std::move(other.ctx_);
            send_id_ = std::move(other.send_id_);
            recv_id_ = std::move(other.recv_id_);

            return *this;
        }

        Pipe(
            std::shared_ptr<Pool> pool,
            const std::size_t buffer_size,
            const std::size_t senders_count,
            const std::size_t receivers_count,
            BroadcastNone)
        {
            initContext(
                std::move(pool), buffer_size, senders_count, receivers_count, pipe_details::BroadcastType::None);
        }

        Pipe(
            std::shared_ptr<Pool> pool,
            const std::size_t buffer_size,
            const std::size_t senders_count,
            const std::size_t receivers_count,
            BroadcastSome) requires(std::is_copy_assignable_v<T>)
        {
            initContext(
                std::move(pool), buffer_size, senders_count, receivers_count, pipe_details::BroadcastType::Some);
        }

        Pipe(
            std::shared_ptr<Pool> pool,
            const std::size_t buffer_size,
            const std::size_t senders_count,
            const std::size_t receivers_count,
            BroadcastAll) requires(std::is_copy_assignable_v<T>)
        {
            initContext(std::move(pool), buffer_size, senders_count, receivers_count, pipe_details::BroadcastType::All);
        }

        ~Pipe()
        {
            destoryContext();
        }

    public:
        template <class TMovedOrCopied>
        typename pipe_details::Context<T>::SendAwaiter operator<<(TMovedOrCopied&& data)
        {
            if (not send_id_.valid())
            {
                if (send_id_.closed())
                    throw std::logic_error("You can no longer send using a pipe with sending turned off.");

                if (ctx_ == nullptr)
                    throw std::logic_error("You can not use a null pipe.");

                send_id_ = ctx_->newSend();

                if (not send_id_.valid())
                    throw std::logic_error("The pipe creates too many senders.");
            }

            return ctx_->send(send_id_, std::forward<TMovedOrCopied>(data));
        }

        typename pipe_details::Context<T>::RecvAwaiter operator>>(T& result)
        {
            if (not recv_id_.valid())
            {
                if (recv_id_.closed())
                    throw std::logic_error("You can no longer receive using a pipe with receiving turned off.");

                if (ctx_ == nullptr)
                    throw std::logic_error("You can not use a null pipe.");

                recv_id_ = ctx_->newRecv();

                if (not recv_id_.valid())
                    throw std::logic_error("The pipe creates too many receivers.");
            }

            return ctx_->recv(recv_id_, result);
        }

        template <class F> requires(not std::is_same_v<std::remove_reference_t<F>, T>)
        RecvFuncAwaiter operator>>(F&& func)
        {
            return {*this, std::forward<F>(func)};
        }

        void closeSend()
        {
            if (ctx_ == nullptr)
                return;

            ctx_->deleteSend(send_id_);
            send_id_.close();
        }

        void closeRecv()
        {
            if (ctx_ == nullptr)
                return;

            ctx_->deleteRecv(recv_id_);
            recv_id_.close();
        }

        void close()
        {
            closeSend();
            closeRecv();
        }

    private:
        void initContext(
            std::shared_ptr<Pool> pool,
            const std::size_t buffer_size,
            const std::size_t senders_count,
            const std::size_t receivers_count,
            const pipe_details::BroadcastType broadcast_type
        )
        {
            if (senders_count == 0 or receivers_count == 0)
                throw std::logic_error("it dosen't make sense to create a pipe without sender or receiver");

            ctx_ = std::make_shared<pipe_details::Context<T>>(
                std::move(pool), buffer_size, senders_count, receivers_count, broadcast_type);
        }

        void destoryContext()
        {
            if (ctx_ == nullptr)
                return;

            ctx_->deleteSend(send_id_);
            ctx_->deleteRecv(recv_id_);

            send_id_ = {};
            recv_id_ = {};
            ctx_ = nullptr;
        }

    private:
        std::shared_ptr<pipe_details::Context<T>> ctx_;

        pipe_details::SendId send_id_;
        pipe_details::RecvId recv_id_;
    };
}
