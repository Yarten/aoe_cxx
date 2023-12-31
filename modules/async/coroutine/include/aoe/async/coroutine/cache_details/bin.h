//
// Created by yarten on 23-12-29.
//

#pragma once

#include <list>
#include <memory>
#include <concurrentqueue.h>
#include "./sized_allocator.h"


namespace aoe::async::coroutine::cache_details
{
    template<class T>
    class Bin
    {
    public:
        using Ptr = std::shared_ptr<Bin>;

        /**
         * \brief Locates this bin in the list, it belongs to the working thread that fills this bin.
         */
        using Locator = typename std::list<Ptr>::iterator;

        /**
         * \brief Stores the locators of bins that will be cleaned up by the working trhead that holds it.
         */
        using CleaningQuene = moodycamel::ConcurrentQueue<Locator>;

    public:
        Bin(const std::size_t block_size, const std::size_t depth)
            : alloc_(block_size, depth)
        {
            setCleaningRate(0.5);
        }

        /**
         * \param rate [0, 1)
         */
        void setCleaningRate(double rate)
        {
            assert(rate >= 0 and rate < 1);
            cleaning_threshold_ = alloc_.capacity() * rate;
        }

        void * tryAllocate()
        {
            return alloc_.tryAllocate();
        }

        void setCleanable(const std::shared_ptr<CleaningQuene> queue, const Locator locator_of_this)
        {
            assert(not cleanable_.load(std::memory_order::relaxed));

            cleaner_.store({}, std::memory_order::relaxed);
            cleaning_queue_ = std::weak_ptr(queue);
            locator_ = locator_of_this;

            cleanable_.store(true, std::memory_order::release);
        }

        void deallocate(void * ptr)
        {
            alloc_.deallocate(ptr);

            if (cleanable_.load(std::memory_order::acquire) and alloc_.count() <= cleaning_threshold_)
            {
                std::thread::id expected_empty_id;

                if (
                    cleaner_.compare_exchange_strong(
                        expected_empty_id,
                        std::this_thread::get_id(),
                        std::memory_order::acquire,
                        std::memory_order::relaxed))
                {
                    cleanable_.store(false, std::memory_order::release);

                    const std::shared_ptr<CleaningQuene> ori_queue = cleaning_queue_.lock();
                    if (ori_queue != nullptr)
                    {
                        if (not ori_queue->enqueue(locator_))
                            throw std::bad_alloc();
                    }

                    cleaning_queue_ = {};
                    locator_ = {};
                }
            }
        }

    private:
        SizedAllocator<T> alloc_;

        // If this bin has ever been filled, a cleanup will be triggered
        // when it has fewer elements than this threshod.
        std::size_t cleaning_threshold_ = 0;

        // This flag is set to true when the working thread fills the bin,
        // and to false when the bin is relative empty and the working thread deletes elements from the bin.
        std::atomic_bool cleanable_ = false;

        // This id is only used to ensure that no more than one thread is cleaning up this bin at the same time.
        std::atomic<std::thread::id> cleaner_ = {};

        // This queue is set by its owner. It is used by other threads to store the locator of this bin.
        // In this way, the bin's owner can be told to clean up it.
        std::weak_ptr<CleaningQuene> cleaning_queue_;

        // Available only when cleaning_queue_ is available.
        Locator locator_;
    };

    template<class T>
    using BinPtr = typename Bin<T>::Ptr;
}
