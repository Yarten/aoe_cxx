//
// Created by yarten on 23-12-26.
//

#pragma once

#include <memory>
#include <coroutine>


namespace aoe::async::coroutine
{
    class Pool;

    /**
     * \brief Called by Pool to initialize the context of this working thread
     * that belongs to Pool.
     */
    void initThisThread(std::shared_ptr<Pool> pool);

    /**
     * \return Pool in use to schedule the current coroutine. The weak_ptr is uninitialzied if this thread
     * is not created by Pool.
     */
    std::weak_ptr<Pool> currentPool() noexcept;

    /**
     * \brief Start to execute a sub coroutine, or return to the father coroutine.
     * If current thread is not a Pool working thread, nothing happens.
     * \return The coroutine that was just executed.
     */
    std::coroutine_handle<> switchTo(std::coroutine_handle<> next_ch) noexcept;

    /**
     * \return The coroutine that is executing now.
     * If this thread is not a Pool working thread, it returns null.
     */
    std::coroutine_handle<> currentHandle() noexcept;

    /**
     * \brief Called by Pool, to suspend coroutine created by Pool, and schedue it later.
     * If a coroutine is created by user (directly call the coroutine), it will not affect its init point.
     */
    void suspendCurrentInitPoint(bool enable) noexcept;

    /**
     * \brief Used by a new coroutine to check if it should suspend at its init point.
     * This option is set through \ref suspendCurrentInitPoint().
     */
    bool isCurrentInitPointSuspended() noexcept;
}
