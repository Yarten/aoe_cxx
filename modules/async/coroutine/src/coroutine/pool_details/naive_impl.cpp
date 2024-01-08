//
// Created by yarten on 24-1-7.
//

#include <ranges>
#include <cmath>
#include "./naive_impl.h"


namespace aoe::async::coroutine::pool_details
{
    NaiveImpl::NaiveImpl(std::shared_ptr<Pool> pool)
        :
        pool_(std::move(pool)),
        workers_(std::max<std::size_t>(param_.default_thread_count, std::thread::hardware_concurrency()))
    {
        for (auto _ : std::views::iota(0ul, workers_.capacity()))
        {
            workers_.push(*this);
        }
    }

    void NaiveImpl::addTask(std::coroutine_handle<Base> handle)
    {
        ready_queue_.enqueue(handle) or throw std::bad_alloc();
    }
}

namespace aoe::async::coroutine::pool_details
{
    NaiveImpl::Worker::Worker(NaiveImpl& master)
        : master_(master), deleter_cache_(master.deleter_cache_.makeCache())
    {
        th_ = std::jthread(
            [this](const std::stop_token& stop_token)
            {
                initThisThread(master_.pool_);

                while (not stop_token.stop_requested())
                {
                    if (not waitForTasks())
                        continue;

                    do
                    {
                        runAllOwnTasks(stop_token);
                    }
                    while (not stop_token.stop_requested() and takeTasksOrStealFromOthers());
                }
            }
        );
    }

    NaiveImpl::Worker::~Worker()
    {
        th_.request_stop();
    }

    bool NaiveImpl::Worker::waitForTasks()
    {
        if (not master_.ready_queue_.wait_dequeue_timed(current_executing_task_, master_.param_.waiting_timeout))
            return false;

        takeTasksToOwnQueue();
        return true;
    }

    void NaiveImpl::Worker::runAllOwnTasks(const std::stop_token& stop_token)
    {
        execution_begin_time_point_.store(std::chrono::steady_clock::now(), std::memory_order::release);

        do
        {
            can_be_stolen_.store(true, std::memory_order::release);
            runonce();
            can_be_stolen_.store(false, std::memory_order::release);
        }
        while (not stop_token.stop_requested() and ready_queue_.try_dequeue(current_executing_task_));
    }

    void NaiveImpl::Worker::takeTasksToOwnQueue()
    {
        // The number of takes should not exceed the average,
        // otherwise it may cause some threads to be idle while tasks are still queued up
        std::size_t max_count = std::min<std::size_t>(
            master_.param_.capacity_of_working_ready_queue,
            std::ceil(
                static_cast<double>(master_.ready_queue_.size_approx()) /
                static_cast<double>(master_.workers_.capacity())
            )
        );

        if (max_count != 0 and current_executing_task_.address() != nullptr)
            --max_count;

        std::coroutine_handle<Base> future_task;

        while (ready_queue_.size_approx() < max_count and master_.ready_queue_.try_dequeue(future_task))
        {
            ready_queue_.enqueue(future_task) or throw std::bad_alloc();
        }

        master_.last_dequeue_time_point_ = std::chrono::steady_clock::now();
    }

    bool NaiveImpl::Worker::takeTasksOrStealFromOthers()
    {
        const std::chrono::steady_clock::time_point curr_time_point = std::chrono::steady_clock::now();

        auto score = [&curr_time_point](
            const std::size_t queue_size, const std::atomic<std::chrono::steady_clock::time_point> & last_time_point)
        {
            return static_cast<double>(queue_size) *
                std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                    curr_time_point - last_time_point.load(std::memory_order::acquire)
                ).count();
        };

        const double global_ready_queue_hunger =
            score(master_.ready_queue_.size_approx(), master_.last_dequeue_time_point_);

        for(Worker & worker : master_.workers_.view())
        {
            if (this == &worker)
                continue;

            if (not worker.can_be_stolen_.load(std::memory_order::acquire))
                continue;

            const double worker_ready_queue_hunger =
                score(worker.ready_queue_.size_approx(), worker.execution_begin_time_point_);

            if (worker_ready_queue_hunger > global_ready_queue_hunger and
                worker.ready_queue_.try_dequeue(current_executing_task_))
                break;
        }

        if (current_executing_task_.address() == nullptr and
            master_.ready_queue_.try_dequeue(current_executing_task_))
            takeTasksToOwnQueue();

        return current_executing_task_.address() != nullptr;
    }

    void NaiveImpl::Worker::runonce()
    {
        assert(current_executing_task_.address() != nullptr);

        do
        {
            Base & ctx = current_executing_task_.promise();

            // If the coroutine is newly created by pool instead of another coroutine,
            // we should maintain its lifetime.
            if (ctx.isInitial())
            {
                CacheElement<Deleter> d = deleter_cache_.next();
                d.construct(current_executing_task_);
                ctx.setCacheDeleter(std::move(d));
            }

            // Execute the coroutine
            current_executing_task_.resume();

            switch (ctx.switchToSuspendedState())
            {
            case State::SuspendBroken:
                // Coroutine suspend is broken, it should be immediately resumed.
                break;

            case State::Ending:
                // The exeuction of the corotuine is finished, we should try to return to its father coroutine.
                current_executing_task_ = std::coroutine_handle<Base>::from_address(ctx.getFatherHandle().address());

                // If this coroutine is the root coroutine, it must be maintained by pool, we should destruct it.
                if (current_executing_task_.address() == nullptr)
                    std::move(ctx).takeCacheDeleter().destruct();
                break;

            default:
                current_executing_task_ = {};
                break;
            }
        } while (current_executing_task_.address() != nullptr);
    }
}
