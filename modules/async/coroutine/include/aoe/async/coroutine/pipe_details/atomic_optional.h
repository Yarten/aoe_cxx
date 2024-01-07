//
// Created by yarten on 24-1-6.
//

#pragma once

#include <atomic>
#include <memory>


namespace aoe::async::coroutine::pipe_details
{
    template<class T>
    class AtomicOptional
    {
    public:
        std::unique_ptr<T> takeAndReset()
        {
            return std::unique_ptr<T>(data_.exchange(nullptr, std::memory_order::acquire));
        }

        void reset()
        {
            data_.store(nullptr, std::memory_order::release);
        }

        template<class ... TArgs>
        void emplace(TArgs &&... args)
        {
            data_.store(new T(std::forward<TArgs>(args)...), std::memory_order::release);
        }

    private:
        std::atomic<T *> data_ = nullptr;
    };
}
