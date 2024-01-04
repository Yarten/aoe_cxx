//
// Created by yarten on 23-12-28.
//

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <functional>
#include <aoe/panic.h>


namespace aoe::async::coroutine::cache_details
{
    /**
     * \brief Allocator that manage fixed size memory to store objects.
     * For efficiency, a allocator is allowed to allocate new cache nodes by only one thread at a time.
     * However, there is no such restriction for deallocating existed cache nodes.
     */
    template<class T, template<class> class TAlloc = std::allocator>
    class SizedAllocator
    {
        struct TreeNode
        {
            std::atomic_size_t count = 0;
        };

        struct CacheNode
        {
            std::atomic_bool used = false;
            std::byte mem[sizeof(T)] {};
        };

        using Alloc = TAlloc<std::byte>;
    public:
        SizedAllocator() noexcept = default;
        SizedAllocator & operator=(SizedAllocator &&) noexcept = delete;

        SizedAllocator(SizedAllocator && other) noexcept
        {
            swap(other);
        }

        SizedAllocator(const std::size_t block_size, const std::size_t depth, Alloc && alloc = Alloc())
            : alloc_(std::forward<Alloc>(alloc)), block_size_(block_size), depth_(depth)
        {
            if (block_size_ == 0 or depth_ == 0)
                return;

            tree_size_         = (1 << depth_) - 1;
            cache_block_count_ = (1 << (depth_ - 1)); // also the leaves count of the tree
            cache_size_        = cache_block_count_ * block_size_;

            const std::size_t tree_mem_size = (tree_size_ + 1) * sizeof(TreeNode);
            mem_size_ = tree_mem_size + cache_size_ * sizeof(CacheNode);

            try
            {
                // [useless 0th tree node | tree structure ... | cache ...]
                mem_ptr_ = alloc_.allocate(mem_size_);
            }
            catch (...)
            {
                block_size_ = depth_ = tree_size_ = cache_block_count_ = cache_size_ = mem_size_ = 0;
                throw;
            }

            tree_ptr_  = new(mem_ptr_) TreeNode[tree_size_ + 1];
            cache_ptr_ = new(mem_ptr_ + tree_mem_size) CacheNode[cache_size_];
        }

        ~SizedAllocator() noexcept
        {
            fastClear();
            alloc_.deallocate(mem_ptr_, mem_size_);
        }

        void swap(SizedAllocator & other) noexcept
        {
            std::swap(alloc_, other.alloc_);

            std::swap(mem_ptr_, other.mem_ptr_);
            std::swap(tree_ptr_, other.tree_ptr_);
            std::swap(cache_ptr_, other.cache_ptr_);

            std::swap(block_size_, other.block_size_);
            std::swap(depth_, other.depth_);

            std::swap(mem_size_, other.mem_size_);
            std::swap(tree_size_, other.tree_size_);
            std::swap(cache_size_, other.cache_size_);
            std::swap(cache_block_count_, other.cache_block_count_);
        }

    public:
        /**
         * \brief Try to allocate a new cache node. This function can only be used by one thread.
         *
         * \return If successful, it returns a pointer to the memory
         * that is used to construct T object by placement new.
         */
        void * tryAllocate()
        {
            if (tree_ptr_ == nullptr)
                return nullptr;

            // Try to enter to subtree by increasing by one to the node's count
            const auto tryEnterSubTree = [](TreeNode & node, std::size_t max_count) -> bool
            {
                if (node.count.load(std::memory_order::acquire) < max_count)
                {
                    node.count.fetch_add(1, std::memory_order::release);
                    return true;
                }

                return false;
            };

            // Scanning from the root node of the count tree all the way to the leaf node,
            // which is the cache block for new object.
            std::size_t tree_node_idx = 1;
            std::size_t max_count = cache_size_;

            if (not tryEnterSubTree(tree_ptr_[tree_node_idx], max_count))
                return nullptr;

            for(std::size_t d = 1; d < depth_; ++d)
            {
                tree_node_idx <<= 1;
                max_count     >>= 1;

                // Enter either left subtree or right subtree of current node
                const bool status =
                    tryEnterSubTree(tree_ptr_[tree_node_idx], max_count) or
                    tryEnterSubTree(tree_ptr_[++tree_node_idx], max_count);

                assert(status);
            }

            // Now tree_node_idx is the leaf index of cache block that is used for the new object,
            // we scan this block to find an empty node
            const std::size_t cache_node_idx = (tree_node_idx - cache_block_count_) * block_size_;

            for(std::size_t i = 0; i < block_size_; ++i)
            {
                CacheNode & node = cache_ptr_[cache_node_idx + i];
                bool expected_not_used = false;

                if (node.used.compare_exchange_strong(
                    expected_not_used,
                    true,
                    std::memory_order::acquire,
                    std::memory_order::relaxed))
                {
                    return static_cast<void*>(node.mem);
                }
            }

            panic.wtf("It must be able to fetch one available cache leaf.");
        }

        /**
         * \brief Destory a cache node. Assume that the object is destructed.
         */
        void deallocate(void * ptr)
        {
            if (ptr == nullptr)
                return;

            // Get the cache node that stores this object
            auto * cache_node_ptr = static_cast<CacheNode*>(ptr - offsetof(CacheNode, mem));

            // Unuse this cache node
            cache_node_ptr->used.store(false, std::memory_order::release);

            // Get this node's leaf index, and prepare to count down all the way to the tree's root
            const std::size_t cache_node_idx = cache_node_ptr - cache_ptr_;

            std::size_t tree_node_idx = cache_node_idx / block_size_ + cache_block_count_;
            assert(tree_node_idx <= tree_size_);

            for(std::size_t i = 0; i < depth_; ++i)
            {
                assert(tree_node_idx != 0);

                tree_ptr_[tree_node_idx].count.fetch_sub(1, std::memory_order::release);
                tree_node_idx >>= 1;

                assert(tree_node_idx != 0 or i + 1 == depth_);
            }
        }

        /**
         * \brief Non-thread-safe, but fast cache cleanup
         */
        void fastClear(
            const std::function<void(T & data)> & destructor =
            [](T & data)
            {
                if constexpr (std::is_class_v<T>)
                    data.~T();
            }
        )
        {
            for(std::size_t idx = 1; idx <= tree_size_; ++idx)
                tree_ptr_[idx].count.store(0, std::memory_order::relaxed);

            for(std::size_t idx = 0; idx < cache_size_; ++idx)
            {
                CacheNode * node = cache_ptr_ + idx;
                bool is_used = true;

                if (node->used.compare_exchange_strong(is_used, false, std::memory_order::relaxed))
                {
                    if (destructor)
                        destructor(*static_cast<T*>(static_cast<void*>(&(node->mem))));
                }
            }
        }

        /**
         * \return The capacity of this fixed size allocator.
         */
        std::size_t capacity() const
        {
            return cache_size_;
        }

        /**
         * \return The current using count of this allocator's memory.
         */
        std::size_t count() const
        {
            if (tree_ptr_ == nullptr)
                return 0;
            else
                return tree_ptr_[1].count.load(std::memory_order::acquire);
        }

    private:
        Alloc alloc_;

        std::byte * mem_ptr_   = nullptr;
        TreeNode  * tree_ptr_  = nullptr;
        CacheNode * cache_ptr_ = nullptr;

        std::size_t block_size_ = 0;
        std::size_t depth_      = 0;

        std::size_t mem_size_   = 0;
        std::size_t tree_size_  = 0; // The real size is tree_size_ + 1, we don't use the 0th node.
        std::size_t cache_size_ = 0;
        std::size_t cache_block_count_ = 0;
    };
}
