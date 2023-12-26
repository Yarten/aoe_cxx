//
// Created by yarten on 23-12-26.
//

#pragma once

#include <coroutine>
#include <exception>

#include "./thread_context.h"


namespace aoe::async::coroutine
{
    /**
     * \brief Define some promises' common behaviors and staticistic data
     */
    struct PromiseBase
    {
        std::exception_ptr exception_ptr;

        /**
         * \brief Always suspend the new coroutine, it is scheduled by Pool.
         */
        static std::suspend_always initial_suspend()
        {
            return {};
        }

        /**
         * \brief Always suspend the dead coroutine, it is destroyed by Pool, or by its father coroutine.
         */
        static std::suspend_always final_suspend()
        {
            return {};
        }

        /**
         * \brief The unhandled exception will be rethrown to this coroutine's father corouine,
         * or cause termination if this coroutine is the root coroutine.
         */
        void unhandled_exception()
        {
            exception_ptr = std::current_exception();
        }
    };

    template<class T>
    struct Promise;

    template<>
    struct Promise<void> : PromiseBase
    {
        /**
         * \brief Create New coroutine handle from this new promise
         */
        auto get_return_object()
        {
            return std::coroutine_handle<Promise>::from_promise(*this);
        }


    };
}
