//
// Created by yarten on 23-12-30.
//

#pragma once

#include <coroutine>
#include <exception>
#include <chrono>

#include <aoe/macro.h>
#include "./thread_context.h"
#include "./cache.h"


namespace aoe::async::coroutine
{
    class Deleter
    {
    public:
        explicit Deleter(const std::coroutine_handle<> handle)
            : handle_(handle)
        {
        }

        Deleter(const Deleter &) = delete;
        Deleter & operator=(const Deleter &) = delete;
        Deleter & operator=(Deleter &&) = delete;

        Deleter(Deleter && other) noexcept
            : handle_(other.handle_)
        {
            other.handle_ = {};
        }

        ~Deleter()
        {
            if (handle_.address() != nullptr)
                handle_.destroy();
        }

    private:
        std::coroutine_handle<> handle_;
    };

    template <class T>
    concept VoidAwaiterTrait = requires(T& obj)
    {
        { obj.onResume() } -> std::same_as<void>;
    };

    enum class State
    {
        // The coroutine is newly created.
        Initial,

        // The coroutine is not running and is not ready to run.
        // This state can only be transformed from the Suspending state.
        Suspended,

        // The coroutine is not running, but is ready to run.
        // This state can only be transformed from the Suspended state.
        Queuing,

        // The coroutine is running on pool, all suspensions have not been performed.
        // This state can be transformed from all other states except the Suspending state.
        Running,

        // The coroutine is still running on pool, but some suspensions are in progress.
        // This state can only be transformed from the Running state.
        Suspending,

        // The coroutine is resumed by other coroutines while it is suspending on pool.
        // When the coroutine suspends, it will be resumed immediately.
        // This state can only be transformed from the Suspending state.
        SuspendBroken,

        // The coroutine reaches its final point, and is waiting for being destoryed.
        // This state is the final state, and can only be transformed from the Running state.
        Ending,
    };

    /**
     * \brief Define some promises' common behaviors and staticistic data
     */
    class Base
    {
    public:
        std::exception_ptr takeUnhandledExeception() noexcept
        {
            return std::move(exception_ptr_); // TODO: 处理异常
        }

        [[nodiscard]]
        std::coroutine_handle<> getFatherHandle() const noexcept
        {
            return father_handle_;
        }

        [[nodiscard]]
        bool isInitial() const
        {
            return state_.load(std::memory_order::release) == State::Initial;
        }

        void setCacheDeleter(CacheElement<Deleter> deleter) noexcept
        {
            cache_deleter_.swap(deleter);
        }

        CacheElement<Deleter> takeCacheDeleter() && noexcept
        {
            return std::move(cache_deleter_);
        }

        [[nodiscard]]
        std::chrono::steady_clock::duration getInitWaitTime() const noexcept
        {
            return init_wait_time_;
        }

        [[nodiscard]]
        std::chrono::steady_clock::duration getWaitTime() const noexcept
        {
            return wait_time_;
        }

        [[nodiscard]]
        std::chrono::steady_clock::duration getQueueTime() const noexcept
        {
            return queue_time_;
        }

        [[nodiscard]]
        std::chrono::steady_clock::duration getRunTime() const noexcept
        {
            return run_time_;
        }

        static Base* pointTo(std::coroutine_handle<>& handle) noexcept
        {
            return &std::coroutine_handle<Base>::from_address(handle.address()).promise();
        }

        friend void awake(std::weak_ptr<Pool> pool, std::coroutine_handle<Base> handle);

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

        // The handle to delete this coroutine that is managed by Pool.
        CacheElement<Deleter> cache_deleter_;

        // The execution state of this corotuine.
        std::atomic<State> state_ = State::Initial;

    private:
        enum class AwaiterType
        {
            Init,
            Run,
            Final
        };

        /**
         * \brief Called before this coroutine can be resumed by other threads.
         */
        void suspendFrom(AwaiterType type) noexcept;

        /**
         * \brief Called after this coroutine can be resumed by other threads.
         */
        void resumeFrom(AwaiterType type) noexcept;

        /**
         * \brief This method is used by Pool to awake this coroutine.
         * \return True only if this coroutine is awakened from the Suspended state,
         * in which case it is added to the Pool's run queue.
         */
        bool setReadyToResume() noexcept;

    private:
        std::chrono::steady_clock::time_point last_time_point_ = std::chrono::steady_clock::now();

        std::chrono::steady_clock::duration init_wait_time_{};
        std::chrono::steady_clock::duration wait_time_{};
        std::chrono::steady_clock::duration queue_time_{};
        std::chrono::steady_clock::duration run_time_{};

    public:
        template <class TDerived, AwaiterType TYPE = AwaiterType::Run>
        class Awaiter
        {
        public:
            explicit Awaiter(const std::coroutine_handle<Base> handle) noexcept
                : handle_(handle)
            {
            }

            explicit Awaiter(const std::coroutine_handle<> handle) noexcept
                : handle_(std::coroutine_handle<Base>::from_address(handle.address()))
            {
            }

            Awaiter(Awaiter&& other) noexcept
                : handle_(other.handle_)
            {
                other.handle_ = {};
            }

            [[nodiscard]]
            bool await_ready() const AOE_NOEXCEPT_BODY(derived().isReady())

            void await_suspend(const std::coroutine_handle<> handle)
                noexcept(noexcept(derived().onSuspend({})) and noexcept(await_ready()))
            {
                if (handle_.address() != nullptr)
                {
                    assert(handle.address() == handle_.address());
                    handle_.promise().suspendFrom(TYPE);
                }

                // let the derived awaiter resume this coroutine at the right time
                derived().onSuspend(std::coroutine_handle<Base>::from_address(handle.address()));

                // Ensure that this coroutine is not suddenly ready because of other coroutines,
                // causing it to hang and never be woken up again.
                if (handle_.address() != nullptr and await_ready())
                    handle_.promise().setReadyToResume();
            }

            void await_resume()
                noexcept(noexcept(derived().onResume()))
                requires(VoidAwaiterTrait<TDerived>)
            {
                switchTo(handle_);

                // let the derived awaiter ensure that this coroutine will not be resumed again.
                derived().onResume();

                if (handle_.address() != nullptr)
                    handle_.promise().resumeFrom(TYPE);
            }

            auto await_resume()
                noexcept(noexcept(derived().onResume()))
                requires(not VoidAwaiterTrait<TDerived>)
            {
                switchTo(handle_);

                auto promised = derived().onResume();

                if (handle_.address() != nullptr)
                    handle_.promise().resumeFrom(TYPE);

                return promised;
            }

            void await_abort()
                noexcept(noexcept(derived().onAbort()))
            {
                switchTo(handle_);

                // let the derived awaiter ensure that this coroutine will not be resumed again.
                derived().onAbort();

                if (handle_.address() != nullptr)
                    handle_.promise().resumeFrom(TYPE);
            }

        private:
            TDerived& derived() noexcept
            {
                return *static_cast<TDerived*>(this);
            }

            [[nodiscard]]
            const TDerived& derived() const noexcept
            {
                return *static_cast<const TDerived*>(this);
            }

        private:
            std::coroutine_handle<Base> handle_;
        };

    private:
        class InitAwaiter : public Awaiter<InitAwaiter, AwaiterType::Init>
        {
        public:
            using Awaiter::Awaiter;

            [[nodiscard]]
            bool isReady() const noexcept
            {
                return not isCurrentInitPointSuspended();
            }

            void onSuspend(std::coroutine_handle<Base>) const noexcept
            {
            }

            void onResume() const noexcept
            {
            }
        };

        class FinalAwaiter : public Awaiter<FinalAwaiter, AwaiterType::Final>
        {
        public:
            using Awaiter::Awaiter;

            [[nodiscard]]
            bool isReady() const noexcept
            {
                return false;
            }

            void onSuspend(std::coroutine_handle<Base>) const noexcept
            {
            }

            void onResume() const noexcept
            {
            }
        };

    public:
        /**
         * \brief Suspend the new coroutine if it is created by Pool.
         */
        [[nodiscard]]
        auto initial_suspend() noexcept
        {
            return InitAwaiter{std::coroutine_handle<Base>::from_promise(*this)};
        }

        /**
         * \brief Always suspend the dead coroutine, it is destroyed by its true holder.
         * Handle.done() will return true in this case.
         */
        [[nodiscard]]
        auto final_suspend() noexcept
        {
            return FinalAwaiter{std::coroutine_handle<Base>::from_promise(*this)};
        }

        /**
         * \brief The unhandled exception will be rethrown to this coroutine's father corouine,
         * or cause termination if this coroutine is the root coroutine.
         */
        void unhandled_exception() noexcept
        {
            exception_ptr_ = std::current_exception();
        }
    };

    template<class TDerived>
    class BoolAwaiter : public Base::Awaiter<BoolAwaiter<TDerived>>
    {
    public:
        using Base::Awaiter<BoolAwaiter>::Awaiter;

        TDerived && operator&(std::function<void()> true_callback) &&
        {
            true_callback_ = std::move(true_callback);
            return std::move(derived());
        }

        TDerived && operator|(std::function<void()> false_callback) &&
        {
            false_callback_ = std::move(false_callback);
            return std::move(derived());
        }

    private:
        friend class Base::Awaiter<BoolAwaiter>;

        [[nodiscard]]
        bool isReady() const AOE_NOEXCEPT_BODY(derived().isReady())

        void onSuspend(const std::coroutine_handle<Base> handle)
        AOE_NOEXCEPT_BODY(derived().onSuspend(handle))

        bool onResume()
        {
            const bool result = derived().onResume();

            if (result)
            {
                if (true_callback_)
                    true_callback_();
            }
            else
            {
                if (false_callback_)
                    false_callback_();
            }

            return result;
        }

        void onAbort()
        {
            derived().onAbort();

            if (false_callback_)
                false_callback_();
        }

    private:
        TDerived& derived() noexcept
        {
            return *static_cast<TDerived*>(this);
        }

        [[nodiscard]]
        const TDerived& derived() const noexcept
        {
            return *static_cast<const TDerived*>(this);
        }

    private:
        std::function<void()> true_callback_;
        std::function<void()> false_callback_;
    };
}
