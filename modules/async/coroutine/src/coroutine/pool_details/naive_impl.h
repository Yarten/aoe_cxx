//
// Created by yarten on 24-1-7.
//

#pragma once

#include <thread>
#include <concurrentqueue.h>
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
        /**
         * \brief This simple implementation uses a fixed number of threads for scheduling,
         * which is related to the maximum number of threads in the hardware.
         */
        NaiveImpl();


        void addTask(std::coroutine_handle<Base> handle);

    private:
        struct Worker
        {
            // the working thread
            std::jthread th;

            // ready tasks for this thread to fetch efficiently, they can be stolen by other idle threads
            moodycamel::ConcurrentQueue<std::coroutine_handle<Base>> ready_queue;

            // part of deleter global cache, cache is spreaded out across threads for efficiency
            Cache<Deleter> deleter_cache;

            explicit Worker(NaiveImpl & master);
        };

    private:
        // maintains the lifetime of corountines that are launched by Pool
        GlobalCache<Deleter> deleter_cache_;

        // coroutines that are ready to run
        moodycamel::ConcurrentQueue<std::coroutine_handle<Base>> ready_queue_;

        // all async working
        SizedStack<Worker> workers_;
    };
}
