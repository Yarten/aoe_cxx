//
// Created by yarten on 23-12-26.
//

#pragma once

#include "./promise.h"


namespace aoe::async::coroutine
{
    template<class T>
    class Holder
    {
    public:
        using promise_type = Promise<T>;
        using Handle       = std::coroutine_handle<promise_type>;

    public:
        Holder(Handle handle)
            : handle_(handle)
        {
        }

    public:
        /**
         * \brief Used by the father coroutine, which is the current coroutine, to check that
         * if this coroutine's promise is ready for the next value.
         */
        bool await_ready() const noexcept
        {

        }

    private:
        Handle handle_;
    };
}
