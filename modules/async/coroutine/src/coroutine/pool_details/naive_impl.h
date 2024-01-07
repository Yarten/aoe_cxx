//
// Created by yarten on 24-1-7.
//

#pragma once

#include <thread>
#include <concurrentqueue.h>
#include <aoe/async/coroutine/base.h>
#include <aoe/async/coroutine/cache.h>


namespace aoe::async::coroutine::pool_details
{
    class NaiveImpl
    {
    public:
        void addTask(Deleter deleter);

    private:
        struct Worker
        {
            std::jthread th;

            moodycamel::ConcurrentQueue<std::coroutine_handle<Base>> ready_queue;
        };

    private:
        // maintains the lifetime of corountines that are launched by Pool
        GlobalCache<Deleter> deleter_cache_;

        // coroutines that are ready to run
        moodycamel::ConcurrentQueue<std::coroutine_handle<Base>> ready_queue_;
    };
}
