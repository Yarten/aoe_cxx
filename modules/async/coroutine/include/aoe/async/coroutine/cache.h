//
// Created by yarten on 23-12-29.
//

#pragma once

#include "./cache_details/bin.h"


namespace aoe::async::coroutine
{
    /**
     * \brief A thread-safe queue that stores spare bins for caches.
     */
    template<class T>
    class BinQueue
    {
        using BinPtr = cache_details::BinPtr<T>;
    public:
        void push(BinPtr && ptr)
        {
            if (ptr == nullptr)
                return;

            if (not bins_.enqueue(std::move(ptr)))
                throw std::bad_alloc();
        }

        BinPtr pop()
        {
            BinPtr ptr;
            bins_.try_dequeue(ptr);
            return ptr;
        }

    private:
        moodycamel::ConcurrentQueue<BinPtr> bins_;
    };

    template<class T>
    class Cache;

    /**
     * \brief The cache memory hodler for a T object.
     */
    template<class T>
    class CacheElement
    {
        using BinPtr = cache_details::BinPtr<T>;
    public:
        CacheElement() = default;
        CacheElement & operator=(CacheElement &&) noexcept = delete;

        CacheElement(CacheElement && other) noexcept
        {
            swap(other);
        }

        void swap(CacheElement & other) noexcept
        {
            std::swap(bin_ptr_, other.bin_ptr_);
            std::swap(mem_ptr_, other.mem_ptr_);
        }

    public:
        template<class ... TArgs>
        T * construct(TArgs && ... args)
        {
            return new(mem_ptr_) T(std::forward<TArgs>(args)...);
        }

        void destruct() &&
        {
            if constexpr (std::is_class_v<T>)
                static_cast<T*>(mem_ptr_)->~T();

            bin_ptr_->deallocate(mem_ptr_);
            mem_ptr_ = nullptr;
        }

    private:
        friend class Cache<T>;

        CacheElement(BinPtr bin_ptr, void * mem_ptr)
            : bin_ptr_((std::move(bin_ptr))), mem_ptr_(mem_ptr)
        {
        }

    private:
        BinPtr bin_ptr_;
        void * mem_ptr_ = nullptr;
    };

    /**
     * \brief A thread-local cache.
     */
    template<class T>
    class Cache
    {
        using BinPtr = typename cache_details::Bin<T>::Ptr;
    public:
        struct Config
        {
            std::size_t bin_block_size = 5;
            std::size_t bin_depth = 4;
        };

    public:
        Cache() = default;
        Cache & operator=(Cache &&) noexcept = delete;

        Cache(Cache && other) noexcept
        {
            swap(other);
        }

        explicit Cache(std::shared_ptr<BinQueue<T>> shared_spare_bins)
            : spare_bins_(std::move(shared_spare_bins))
        {
        }

        Cache(std::shared_ptr<BinQueue<T>> shared_spare_bins, Config config)
            : config_(config), spare_bins_(std::move(shared_spare_bins))
        {
        }

        void swap(Cache & other) noexcept
        {
            std::swap(config_, other.config_);
            std::swap(spare_bins_, other.spare_bins_);
            std::swap(bins_, other.bins_);
            std::swap(cleaning_quene_, other.cleaning_quene_);
        }

    public:
        /**
         * \brief Create a new cache element. It's the user's responsibility to release this cache element
         * by calling its destruct() function.
         * This function can only be used by one thread.
         */
        CacheElement<T> next()
        {
            // create the first bin ever
            if (bins_.empty())
                createNewBin();

            // start by trying this cache's bin
            void * ptr = bins_.front()->tryAllocate();

            if (ptr != nullptr)
                return {bins_.front(), ptr};

            // the bin is full, it should wait to be cleaned up
            bins_.front()->setCleanable(cleaning_quene_, bins_.begin());

            // now we should find a useable bin either from the cleaning queue, or the spare queue.
            std::shared_ptr<BinQueue<T>> shared_spare_bins = spare_bins_.lock();
            bool has_bin_to_reuse = false;

            if (shared_spare_bins == nullptr)
                has_bin_to_reuse = reuseBinFromCleanningQueue();
            else
                has_bin_to_reuse = reuseBinFromSpareQueue(std::move(shared_spare_bins));

            // there is no bin to reuse, we'll just have to create a new one
            if (not has_bin_to_reuse)
                createNewBin();

            // now, the front bin must be available
            ptr = bins_.front()->tryAllocate();

            if (ptr == nullptr)
                throw std::bad_alloc();

            return {bins_.front(), ptr};
        }


        void cleanup()
        {

        }

        void forceCleanup()
        {

        }

    private:
        void createNewBin()
        {
            bins_.push_front(
                std::make_shared<cache_details::Bin<T>>(
                    config_.bin_block_size, config_.bin_depth
                )
            );
        }

        bool reuseBinFromSpareQueue(std::shared_ptr<BinQueue<T>> shared_spare_bins)
        {
            // we first clean up bins that are relative empty,
            // leaving only one to be used and giving the rest to the spare queue.
            if (reuseBinFromCleanningQueue())
            {
                typename cache_details::Bin<T>::Locator it;

                while (cleaning_quene_->try_dequeue(it))
                {
                    shared_spare_bins->push(std::move(*it));
                    bins_.erase(it);
                }

                return true;
            }

            // if no bins can be reused from the cleaning queue, try to get a bin from the spare queue
            BinPtr ptr = shared_spare_bins->pop();

            if (ptr == nullptr)
                return false;

            bins_.push_front(std::move(ptr));
            return true;
        }

        bool reuseBinFromCleanningQueue()
        {
            if (typename cache_details::Bin<T>::Locator it; cleaning_quene_->try_dequeue(it))
            {
                bins_.push_front(std::move(*it));
                bins_.erase(it);
                return true;
            }

            return false;
        }

    private:
        Config config_;

        // When this cache needs a new bin, it will first check to see if there are any spare bins left,
        // and only create a new one if there aren't any.
        std::weak_ptr<BinQueue<T>> spare_bins_;

        // Bins that belong to this cache for now. Only this cache can allocate new memory on those bins.
        // The front bin in the list is the one bing used.
        // The reset of the bins have been filled and are waiting to be cleaned up when they are relative empty.
        std::list<BinPtr> bins_;

        // This queue stores the list's iterators of bins that are ready to be cleaned up.
        using CleaningQueue = typename cache_details::Bin<T>::CleaningQuene;
        std::shared_ptr<CleaningQueue> cleaning_quene_ = std::make_shared<CleaningQueue>();
    };

    /**
     * \brief A proxy class that can create multi thread-local caches,
     * sharing the same spare bins queue that is owned by this class.
     */
    template<class T>
    class GlobalCache
    {
    public:
        Cache<T> makeCache(typename Cache<T>::Config config = {})
        {
            return {spare_bins_, config};
        }

    private:
        std::shared_ptr<BinQueue<T>> spare_bins_ = std::make_shared<BinQueue<T>>();
    };
}
