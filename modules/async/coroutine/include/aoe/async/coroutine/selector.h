//
// Created by yarten on 24-1-3.
//

#pragma once

#include <tuple>
#include "./base.h"


namespace aoe::async::coroutine
{
    template<class ... TAwaiter>
    class Selector : public BoolAwaiter<Selector<TAwaiter...>>
    {
    public:
        explicit Selector(TAwaiter && ... awaiter)
            : awaiters_(std::forward<TAwaiter>(awaiter)...)
        {
        }

    public:
        [[nodiscard]]
        bool isReady() const
            noexcept(noexcept(isReady(std::make_index_sequence<sizeof...(TAwaiter)>())))
        {
            return isReady(std::make_index_sequence<sizeof...(TAwaiter)>());
        }

        void onSuspend(std::coroutine_handle<Base> handle)
        {

        }

    private:
        template<std::size_t ... I>
        [[nodiscard]] bool isReady(std::index_sequence<I...>) const
            noexcept(noexcept((... or std::get<I>(awaiters_).isReady())))
        {
            return (... or std::get<I>(awaiters_).isReady());
        }

        template<std::size_t ... I>
        void onSuspend(std::coroutine_handle<Base> handle, std::index_sequence<I...>)
            noexcept(noexcept((std::get<I>(awaiters_).onSuspend(handle), ...)))
        {
            (std::get<I>(awaiters_).onSuspend(handle), ...);
        }

    private:
        std::tuple<TAwaiter && ...> awaiters_;
    };
}
