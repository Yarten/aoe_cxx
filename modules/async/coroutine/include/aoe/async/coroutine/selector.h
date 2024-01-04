//
// Created by yarten on 24-1-3.
//

#pragma once

#include <tuple>
#include "./base.h"


namespace aoe::async::coroutine
{
    template <class... TAwaiter>
    class Selector : public BoolAwaiter<Selector<TAwaiter...>>
    {
        using Super = BoolAwaiter<Selector>;
        using Indexes = std::make_index_sequence<sizeof...(TAwaiter)>;

    public:
        explicit Selector(TAwaiter&&... awaiter)
            : Super(currentHandle()), awaiters_(std::forward<TAwaiter>(awaiter)...)
        {
        }

    public:
        [[nodiscard]]
        bool isReady() const
        AOE_NOEXCEPT_BODY(
            isReady(Indexes())
        )

        void onSuspend(std::coroutine_handle<Base> handle)
        AOE_NOEXCEPT_BODY(
            onSuspend(handle, Indexes())
        )

        bool onResume()
        {
            return onResume(Indexes());
        }

        void onAbort()
        {
            onAbort(Indexes());
        }

    private:
        template <std::size_t ... I>
        [[nodiscard]] bool isReady(std::index_sequence<I...>) const
        AOE_NOEXCEPT_BODY(
            (std::get<I>(awaiters_).isReady() or ...)
        )

        template <std::size_t ... I>
        void onSuspend(std::coroutine_handle<Base> handle, std::index_sequence<I...>)
        AOE_NOEXCEPT_BODY(
            (std::get<I>(awaiters_).onSuspend(handle), ...)
        )

        template <std::size_t ... I>
        bool onResume(std::index_sequence<I...>)
        {
            bool success = false;
            bool some_ready = false;

            (
                [&](auto && i)
                {
                    if (some_ready or not i.isReady())
                        i.await_abort();
                    else
                    {
                        some_ready = true;
                        success = i.await_resume();
                    }
                }
                (std::get<I>(awaiters_))
                , ...);

            return success;
        }

        template <std::size_t ... I>
        void onAbort(std::index_sequence<I...>)
        {
            (std::get<I>(awaiters_).onAbort(), ...);
        }

    private:
        std::tuple<TAwaiter&&...> awaiters_;
    };
}
