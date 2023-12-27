//
// Created by yarten on 23-12-26.
//

#pragma once

#include <coroutine>
#include <exception>
#include <optional>

#include "./thread_context.h"


namespace aoe::async::coroutine
{
    /**
     * \brief Define some promises' common behaviors and staticistic data
     */
    class PromiseBase
    {
        struct InitAwaiter
        {
            bool await_ready() const noexcept
            {
                return not isCurrentInitPointSuspended();
            }

            void await_suspend(std::coroutine_handle<>) const noexcept {}

            void await_resume() const noexcept {}
        };
    public:
        /**
         * \brief Suspend the new coroutine if it is created by Pool.
         */
        InitAwaiter initial_suspend() const noexcept
        {
            return {};
        }

        /**
         * \brief Always suspend the dead coroutine, it is destroyed by its true holder.
         * Handle.done() will return true in this case.
         */
        std::suspend_always final_suspend() const noexcept
        {
            return {};
        }

        /**
         * \brief The unhandled exception will be rethrown to this coroutine's father corouine,
         * or cause termination if this coroutine is the root coroutine.
         */
        void unhandled_exception() noexcept
        {
            exception_ptr_ = std::current_exception();
        }

    public:
        std::exception_ptr takeUnhandledExeception() noexcept
        {
            return std::move(exception_ptr_); // TODO: 处理异常
        }

        std::coroutine_handle<> getFatherHandle() const noexcept
        {
            return father_handle_;
        }

    protected:
        void recordFatherHandleAndSwitchToThis(const std::coroutine_handle<> self) noexcept
        {
            // This coroutine executes immediately after it is created,
            // unless it was created by a Pool,
            // in which case the context variable will be restored by the Pool.
            father_handle_ = switchTo(self);
        }

    private:
        // The father coroutine that calls this coroutine. When this coroutine yields, its
        // father coroutine will be executed.
        std::coroutine_handle<> father_handle_;

        // The execpetion that is thrown during coroutine's execution
        std::exception_ptr exception_ptr_;
    };

    /**
     * \brief The context of a coroutine.
     */
    template<class TYield, class TRet>
    class Promise;

    template<>
    class Promise<void, void> : public PromiseBase
    {
    public:
        auto get_return_object()
        {
            auto r = std::coroutine_handle<Promise>::from_promise(*this);
            recordFatherHandleAndSwitchToThis(r);
            return r;
        }

        void return_void() noexcept
        {
            is_ready_ = true;
        }

    public:
        bool isReady() const noexcept
        {
            return is_ready_;
        }

        void take() noexcept
        {
            is_ready_ = false;
        }

    private:
        bool is_ready_ = false;
    };

    template<class TYield>
    class Promise<TYield, void> : public PromiseBase
    {
    public:
        auto get_return_object()
        {
            auto r = std::coroutine_handle<Promise>::from_promise(*this);
            recordFatherHandleAndSwitchToThis(r);
            return r;
        }

        void return_void() noexcept
        {
            is_done_ = true;
        }

        std::suspend_always yield_value(TYield value) noexcept
        {
            value_opt_ = std::move(value);
            return {};
        }

    public:
        bool isDone() const noexcept
        {
            return is_done_;
        }

        bool isReady() const noexcept
        {
            return value_opt_.has_value();
        }

        TYield take() noexcept
        {
            std::optional<TYield> r;
            r.swap(value_opt_);
            return std::move(r).value();
        }

    private:
        std::optional<TYield> value_opt_;

        bool is_done_ = false;
    };

    template<class TRet>
    class Promise<void, TRet> : public PromiseBase
    {
    public:
        auto get_return_object()
        {
            auto r = std::coroutine_handle<Promise>::from_promise(*this);
            recordFatherHandleAndSwitchToThis(r);
            return r;
        }

        void return_value(TRet value) noexcept
        {
            value_opt_ = std::move(value);
        }

    public:
        bool isReady() const noexcept
        {
            return value_opt_.has_value();
        }

        TRet take() noexcept
        {
            std::optional<TRet> r;
            r.swap(value_opt_);
            return std::move(r).value();
        }

    private:
        std::optional<TRet> value_opt_;
    };
}
