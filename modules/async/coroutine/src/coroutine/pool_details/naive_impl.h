//
// Created by yarten on 24-1-7.
//

#pragma once

#include <thread>
#include <atomic>
#include <concurrentqueue.h>
#include <blockingconcurrentqueue.h>
#include <aoe/async/coroutine/base.h>
#include <aoe/async/coroutine/cache.h>
#include <aoe/async/coroutine/pipe_details/sized_stack.h>


namespace aoe::async::coroutine::pool_details
{
    template<class T>
    using SizedStack = pipe_details::SizedStack<T>;

    class NaiveImpl
    {
    public:
        struct Param
        {
            // timeout for each thread to wait for a new task to arrive
            std::chrono::duration<std::int64_t> waiting_timeout = std::chrono::seconds(1);

            // The maximum number of tasks that a worker can preemptively take from the global ready queue.
            std::size_t capacity_of_working_ready_queue = 3;

            // We use std::max(std::thread::hardware_concurrency(), param.default_thread_count)
            // as the count of workers.
            std::size_t default_thread_count = 2;
        };

    public:
        /**
         * \brief This simple implementation uses a fixed number of threads for scheduling,
         * which is related to the maximum number of threads in the hardware.
         */
        explicit NaiveImpl(std::shared_ptr<Pool> pool);

        /**
         * \brief Add new created coroutine to the global ready queue
         */
        void addTask(std::coroutine_handle<Base> handle);

    private:
        class Worker
        {
        public:
            explicit Worker(NaiveImpl & master);

            ~Worker();

        private:
            /**
             * \brief Blocks this thread until either new tasks come or the timeout expires.
             * If new tasks arrive, the worker puts some of them into its own ready queue.
             * \return Whether new tasks are available
             */
            bool waitForTasks();

            /**
             * \brief Takes out and runs the worker's own tasks
             */
            void runAllOwnTasks(const std::stop_token& stop_token);

            /**
             * \brief Takes out tasks from the global ready queue, or steals one from other workers' own
             * ready queues. It depends on the hunger of those ready queues.
             * \return Whether there is a task to execute
             */
            bool takeTasksOrStealFromOthers();

        private:
            /**
             * \brief Takes some tasks from the global ready queue.
             */
            void takeTasksToOwnQueue();

            /**
             * \brief Handle the current executing task on this thread
             */
            void runonce();

        private:
            NaiveImpl & master_;

            // the working thread
            std::jthread th_;

            // part of deleter global cache, cache is spreaded out across threads for efficiency
            Cache<Deleter> deleter_cache_;

            // ready tasks for this thread to fetch efficiently, they can be stolen by other idle threads
            moodycamel::ConcurrentQueue<std::coroutine_handle<Base>> ready_queue_;

            // for evaluation the hunger of this worker's ready queue
            std::atomic<std::chrono::steady_clock::time_point> execution_begin_time_point_ = std::chrono::steady_clock::now();

            // worker's tasks can only be stolen when it is executing a coroutine.
            std::atomic<bool> can_be_stolen_ = false;

            // the task is either taken from this worker's ready queue, or stolen from another worker's ready queue.
            std::coroutine_handle<Base> current_executing_task_;
        };

    private:
        Param param_;

        std::shared_ptr<Pool> pool_;

        // all async working
        SizedStack<Worker> workers_;

        // maintains the lifetime of corountines that are launched by Pool
        GlobalCache<Deleter> deleter_cache_;

        // coroutines that are ready to run, idle threads will wait for it
        moodycamel::BlockingConcurrentQueue<std::coroutine_handle<Base>> ready_queue_;

        // For evaluating the hunger of the global ready queue
        std::atomic<std::chrono::steady_clock::time_point> last_dequeue_time_point_ = std::chrono::steady_clock::now();
    };
}
