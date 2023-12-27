//
// Created by yarten on 23-12-27.
//

#include <cassert>
#include <aoe/async/coroutine/thread_context.h>


namespace aoe::async::coroutine
{
    /**
     * \brief The context of working thread that is created by Pool.
     */
    struct ThreadContext
    {
        // The pool that uses this thread to schedule coroutines
        std::weak_ptr<Pool> pool;

        // The coroutine that is executing on this working thread now.
        std::coroutine_handle<> running_coroutine;
    };

    /**
     * \brief Created by Pool, which is the context of Pool's working thread.
     */
    thread_local std::unique_ptr<ThreadContext> thctx_ptr;

    /**
     * \brief Some options to control coroutines' behaviros.
     * They are created by system with default initialization, and are managed later by Pool.
     */
    struct ThreadOptions
    {
        bool shouldSuspendInitPoint = false;
    };

    thread_local ThreadOptions thopts;
}

namespace aoe::async::coroutine
{
    void initThisThread(const std::shared_ptr<Pool> pool)
    {
        assert(thctx_ptr == nullptr);
        thctx_ptr = std::make_unique<ThreadContext>();
        thctx_ptr->pool = std::weak_ptr(pool);
    }

    std::coroutine_handle<> switchTo(const std::coroutine_handle<> next_ch) noexcept
    {
        if (thctx_ptr == nullptr)
            return {};

        const std::coroutine_handle<> r = thctx_ptr->running_coroutine;
        thctx_ptr->running_coroutine = next_ch;
        return r;
    }

    std::weak_ptr<Pool> currentPool() noexcept
    {
        if (thctx_ptr != nullptr)
            return thctx_ptr->pool;

        return {};
    }

    void suspendCurrentInitPoint(const bool enable) noexcept
    {
        thopts.shouldSuspendInitPoint = enable;
    }

    bool isCurrentInitPointSuspended() noexcept
    {
        return thopts.shouldSuspendInitPoint;
    }
}
