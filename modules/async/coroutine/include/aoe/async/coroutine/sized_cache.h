//
// Created by yarten on 23-12-28.
//

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <functional>


namespace aoe::async::coroutine
{
    /**
     * \brief Fixed size cache to store coroutine handles.
     * For efficiency, a cache is allowed to be stored new values by only one thread at a time.
     * However, there is no such restriction for deleting values.
     */
    template<class T>
    class SizedCache
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
    public:
        SizedCache() noexcept = default;
        SizedCache(SizedCache &&) noexcept = default;
        SizedCache & operator=(SizedCache &&) noexcept = default;

        SizedCache(const std::size_t block_size, const std::size_t depth)
            : block_size_(block_size), depth_(depth)
        {
            if (block_size_ == 0 or depth_ == 0)
                return;

            tree_size_         = (1 << depth_) - 1;
            cache_block_count_ = (1 << (depth_ - 1)); // also the leaves count of the tree
            cache_size_        = cache_block_count_ * block_size_;

            const std::size_t tree_mem_size = (tree_size_ + 1) * sizeof(TreeNode);
            mem_size_ = tree_mem_size + cache_size_ * sizeof(CacheNode);

            // [useless 0th tree node | tree structure ... | cache ...]
            mem_ptr_ = new(std::nothrow) std::byte[mem_size_];

            if (mem_ptr_ == nullptr)
            {
                block_size_ = depth_ = tree_size_ = cache_block_count_ = cache_size_ = mem_size_ = 0;
                throw std::bad_alloc();
            }

            tree_ptr_  = new(mem_ptr_) TreeNode[tree_size_ + 1];
            cache_ptr_ = new(mem_ptr_ + tree_mem_size) CacheNode[cache_size_];
        }

        ~SizedCache() noexcept
        {
            fastClear();
            delete[] mem_ptr_;
        }

    public:
        /**
         * \brief Try to cache an new T object. This function can only be used by one thread.
         *
         * \param creator Functor that is responsible to use placement new to create T object.
         *
         * \return If successful, it returns a pointer to the new object.
         */
        T * tryNew(const std::function<void(void * mem_ptr)> & creator)
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
                bool is_used = false;

                if (node.used.compare_exchange_strong(
                    is_used, true, std::memory_order::acquire, std::memory_order::relaxed))
                {
                    auto * mem_ptr = static_cast<void*>(node.mem);
                    creator(mem_ptr);
                    return static_cast<T*>(mem_ptr);
                }
            }

            std::abort(); // Impossible
        }

        /**
         * \brief Destory a object that is stored by this cache.
         */
        void release(
            T *& ptr,
            const std::function<void(T & data)> & deleter =
            [](T & data)
            {
                if constexpr (std::is_class_v<T>)
                    data.~T();
            }
        )
        {
            if (ptr == nullptr)
                return;

            if (deleter)
                deleter(*ptr);

            auto * void_ptr = static_cast<void*>(ptr);
            release(void_ptr);
            ptr = nullptr;
        }

        /**
         * \brief Destory a cache node. Assume that the object is destructed.
         */
        void release(void *& ptr)
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

            ptr = nullptr;
        }

        /**
         * \brief Non-thread-safe, but fast cache cleanup
         */
        void fastClear(
            const std::function<void(T & data)> & deleter =
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
                    if (deleter)
                        deleter(*static_cast<T*>(static_cast<void*>(&(node->mem))));
                }
            }
        }

    private:
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
