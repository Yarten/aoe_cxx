//
// Created by yarten on 24-1-1.
//

#include <aoe/async/coroutine/base.h>


namespace aoe::async::coroutine
{
    State Base::switchToSuspendedState()
    {
        auto original_state = State::Suspending;

        return state_.compare_exchange_strong(
            original_state,
            State::Suspended,
            std::memory_order::acquire,
            std::memory_order::relaxed
        ) ? State::Suspended : original_state;
    }

    void Base::suspendFrom(const AwaiterType type) noexcept
    {
        if (type == AwaiterType::Init)
            return;

        const std::chrono::steady_clock::time_point curr_time_point = std::chrono::steady_clock::now();
        run_time_ += (curr_time_point - last_time_point_);
        last_time_point_ = curr_time_point;

        state_.store(
            type == AwaiterType::Run ? State::Suspending : State::Ending, std::memory_order::release);
    }

    void Base::resumeFrom(const AwaiterType type) noexcept
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
                if (state_.load(std::memory_order::release) == State::Queuing)
                    queue_time_ += interval;
                else
                    wait_time_ += interval;
            }
            break;
        case AwaiterType::Final:
            break;
        }

        state_.store(State::Running, std::memory_order::acquire);
    }

    bool Base::setReadyToResume() noexcept
    {
        State state;

        do
        {
            // awake this coroutine if it is suspended
            if (state = State::Suspended;
                state_.compare_exchange_strong(
                state,
                State::Queuing,
                std::memory_order::acquire,
                std::memory_order::relaxed
            ))
            {
                const std::chrono::steady_clock::time_point curr_time_point = std::chrono::steady_clock::now();
                wait_time_ += (curr_time_point - last_time_point_);
                last_time_point_ = curr_time_point;
                return true;
            }

            // resume this coroutine immediately when it finshes suspending
            state = State::Suspending;
            state_.compare_exchange_strong(
                state,
                State::SuspendBroken,
                std::memory_order::acquire,
                std::memory_order::relaxed
            );

            // after the above operations, the state must not be State::Suspended
        } while (state == State::Suspended);

        // the case that coroutine is queuing or suspend broken
        return false;
    }
}
