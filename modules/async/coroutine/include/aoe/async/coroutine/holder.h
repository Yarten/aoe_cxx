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
    class Holder<void, TRet> :
        public Base::Awaiter<Holder<void, TRet>>
    {
        // A coroutine that will return finally can be awaited by its father coroutine.
        // When this coroutine holder's await functions are called,
        // its fahter coroutine's state will be updated. (available only in Pool)
        using ThisAsAwaiter = Base::Awaiter<Holder>;
    public:
        using promise_type = Promise<void, TRet>;
        using Handle       = std::coroutine_handle<promise_type>;

    public:
        Holder(Handle handle) noexcept
            : ThisAsAwaiter(handle.promise().getFatherHandle()), handle_(handle)
        {
        }

        Holder(Holder && other) noexcept
            : ThisAsAwaiter(std::forward<Holder>(other)), handle_(other.handle_)
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

        std::coroutine_handle<Base> intoHandle() &&
        {
            auto result = std::coroutine_handle<Base>::from_address(handle_.address());
            handle_ = {};
            return result;
        }

    public:
        /**
         * \brief Used by the father coroutine, which is the current coroutine, to check that
         * if this coroutine's promise is ready for the next value, and if not, current coroutine is suspended.
         */
        [[nodiscard]]
        bool isReady() const noexcept
        {
            return handle_.promise().isReady();
        }

        void onSuspend(std::coroutine_handle<Base>) noexcept
        {
        }

        /**
         * \brief Resume the father coroutine
         * \return this coroutine's promised value
         */
        TRet onResume() noexcept
        {
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
        class YieldAwaiter : public BoolAwaiter<YieldAwaiter>
        {
            using Super = BoolAwaiter<YieldAwaiter>;
        public:
            [[nodiscard]]
            bool isReady() const
            {
                if (self_.handle_.promise().isDone())
                    return true;

                if (not self_.handle_.promise().isReady())
                    self_.handle_.resume();

                return self_.handle_.promise().isDone() or self_.handle_.promise().isReady();
            }

            void onSuspend(std::coroutine_handle<Base>) noexcept
            {
            }

            bool onResume() noexcept
            {
                if (not self_.handle_.promise().isDone())
                {
                    result_ = self_.handle_.promise().take();
                    return true;
                }

                return false;
            }

        private:
            YieldAwaiter(Holder & self, TYield & result)
                : Super(self.handle_.promise().getFatherHandle()), self_(self), result_(result)
            {
            }

            friend class Holder;

        private:
            Holder & self_;
            TYield & result_;
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
