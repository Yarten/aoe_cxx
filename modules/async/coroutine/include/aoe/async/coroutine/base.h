//
// Created by yarten on 23-12-30.
//

#pragma once

#include <coroutine>
#include <exception>
#include <chrono>

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

        ~Deleter()
        {
            if (handle_.address() != nullptr)
                handle_.destroy();
        }

    private:
        std::coroutine_handle<> handle_;
    };

    template <class T>
    concept VoidAwaiter = requires(T& obj)
    {
        { obj.onResume() } -> std::same_as<void>;
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
        void suspendFrom(const AwaiterType type) noexcept
        {
            if (type == AwaiterType::Init)
                return;

            const std::chrono::steady_clock::time_point curr_time_point = std::chrono::steady_clock::now();
            run_time_ += (curr_time_point - last_time_point_);
            last_time_point_ = curr_time_point;

            is_ready_to_resume_.store(false, std::memory_order::release);
        }

        /**
         * \brief Called after this coroutine can be resumed by other threads.
         */
        void resumeFrom(const AwaiterType type) noexcept
        {
            const std::chrono::steady_clock::time_point curr_time_point = std::chrono::steady_clock::now();
            const std::chrono::steady_clock::duration interval = curr_time_point - last_time_point_;
            last_time_point_ = curr_time_point;

            switch (type)
            {
            case AwaiterType::Init:
                init_wait_time_ += interval;
                break;
            case AwaiterType::Run:
                {
                    // the sync-relation is built by awaiters, in other words, no awaiter can resume this coroutine
                    // when this coroutine is already awake.
                    if (is_ready_to_resume_.load(std::memory_order::relaxed))
                        queue_time_ += interval;
                    else
                        wait_time_ += interval;
                }
                break;
            case AwaiterType::Final:
                break;
            }
        }

        /**
         * \brief Called by Pool when some threads resume this coroutine while the Pool
         * has no idle thread to run the coroutine.
         */
        void setReadyToResume() noexcept
        {
            bool expected_not_ready_yet = false;

            if (is_ready_to_resume_.compare_exchange_strong(
                expected_not_ready_yet,
                true,
                std::memory_order::acquire,
                std::memory_order::relaxed
            ))
            {
                const std::chrono::steady_clock::time_point curr_time_point = std::chrono::steady_clock::now();
                wait_time_ += (curr_time_point - last_time_point_);
                last_time_point_ = curr_time_point;
            }
        }

    private:
        std::chrono::steady_clock::time_point last_time_point_ = std::chrono::steady_clock::now();

        std::chrono::steady_clock::duration init_wait_time_{};
        std::chrono::steady_clock::duration wait_time_{};
        std::chrono::steady_clock::duration queue_time_{};
        std::chrono::steady_clock::duration run_time_{};

        std::atomic_bool is_ready_to_resume_ = false;

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
            bool await_ready() const noexcept(noexcept(derived().isReady()))
            {
                return derived().isReady();
            }

            void await_suspend(const std::coroutine_handle<> handle) noexcept(noexcept(derived().onSuspend({})))
            {
                if (handle_.address() != nullptr)
                {
                    assert(handle.address() == handle_.address());
                    handle_.promise().suspendFrom(TYPE);
                }

                // let the derived awaiter resume this coroutine at the right time
                derived().onSuspend(handle);
            }

            void await_resume()
                noexcept(noexcept(derived().onResume()))
                requires(VoidAwaiter<TDerived>)
            {
                // let the derived awaiter ensure that this coroutine will not be resumed again.
                derived().onResume();

                if (handle_.address() != nullptr)
                    handle_.promise().resumeFrom(TYPE);
            }

            auto await_resume()
                noexcept(noexcept(derived().onResume()))
                requires(not VoidAwaiter<TDerived>)
            {
                auto promised = derived().onResume();

                if (handle_.address() != nullptr)
                    handle_.promise().resumeFrom(TYPE);

                return promised;
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

            void onSuspend(std::coroutine_handle<>) const noexcept
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

            void onSuspend(std::coroutine_handle<>) const noexcept
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
}
