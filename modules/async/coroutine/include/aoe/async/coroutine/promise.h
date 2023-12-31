//
// Created by yarten on 23-12-26.
//

#pragma once

#include <coroutine>
#include <exception>
#include <optional>

#include "./base.h"


namespace aoe::async::coroutine
{
    /**
     * \brief The context of a coroutine.
     */
    template<class TYield, class TRet>
    class Promise;

    template<>
    class Promise<void, void> : public Base
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
    class Promise<TYield, void> : public Base
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
    class Promise<void, TRet> : public Base
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
