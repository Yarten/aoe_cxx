//
// Created by yarten on 23-12-26.
//

#pragma once

#include "./promise.h"


namespace aoe::async::coroutine
{
    template<class TYield, class TRet>
    class Holder;

    template<class TRet>
    class Holder<void, TRet>
    {
    public:
        using promise_type = Promise<void, TRet>;
        using Handle       = std::coroutine_handle<promise_type>;

    public:
        Holder(Handle handle) noexcept
            : handle_(handle)
        {
        }

        Holder(Holder && other) noexcept
            : handle_(other.handle_)
        {
            other.handle_ = {};
        }

        ~Holder()
        {
            if (handle_.address() != nullptr)
                handle_.destroy();
        }

        Holder(const Holder &) = delete;
        Holder & operator=(const Holder &) = delete;
        Holder & operator=(Holder &&) = delete;

    public:
        /**
         * \brief Used by the father coroutine, which is the current coroutine, to check that
         * if this coroutine's promise is ready for the next value, and if not, current coroutine is suspended.
         */
        bool await_ready() const noexcept
        {
            return handle_.promise().isReady();
        }

        void await_suspend(std::coroutine_handle<>) noexcept
        {
        }

        /**
         * \brief Resume the father coroutine
         * \return this coroutine's promised value
         */
        TRet await_resume() noexcept
        {
            switchTo(handle_.promise().getFatherHandle());
            return handle_.promise().take();
        }

    private:
        // The coroutine handle
        Handle handle_;
    };

    template<class TYield>
    requires (not std::is_void_v<TYield>)
    class Holder<TYield, void>
    {
    public:
        using promise_type = Promise<TYield, void>;
        using Handle       = std::coroutine_handle<promise_type>;

    public:
        Holder(Handle handle) noexcept
            : handle_(handle)
        {
        }

        Holder(Holder && other) noexcept
            : handle_(other.handle_)
        {
            other.handle_ = {};
        }

        ~Holder()
        {
            if (handle_.address() != nullptr)
                handle_.destroy();
        }

        Holder(const Holder &) = delete;
        Holder & operator=(const Holder &) = delete;
        Holder & operator=(Holder &&) = delete;

    private:
        struct YieldAwaiter
        {
            Holder & self;
            TYield & result;

            bool await_ready() const
            {
                if (self.handle_.promise().isDone())
                    return true;

                if (not self.handle_.promise().isReady())
                    self.handle_.resume();

                return self.handle_.promise().isDone() or self.handle_.promise().isReady();
            }

            void await_suspend(std::coroutine_handle<>) noexcept
            {
            }

            bool await_resume() noexcept
            {
                switchTo(self.handle_.promise().getFatherHandle());

                if (not self.handle_.promise().isDone())
                {
                    result = self.handle_.promise().take();
                    return true;
                }

                return false;
            }
        };

    public:
        YieldAwaiter operator>>(TYield & result)
        {
            return {*this, result};
        }

    private:
        // The coroutine handle
        Handle handle_;
    };
}
