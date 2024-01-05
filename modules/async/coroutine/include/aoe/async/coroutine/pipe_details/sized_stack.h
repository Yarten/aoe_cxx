//
// Created by yarten on 24-1-5.
//

#pragma once

#include <vector>
#include <type_traits>
#include <aoe/trait.h>


namespace aoe::async::coroutine::pipe_details
{
    template<class T>
    class SizedStack
    {
        struct Node
        {
            std::byte mem[sizeof(T)] {};
        };
    public:
        explicit SizedStack(const std::size_t max_size)
            : nodes_(max_size)
        {
        }

        template<class ... TArgs>
        T & push(TArgs &&... args)
        {
            return *new(nodes_[size_++].mem) T(std::forward<TArgs>(args)...);
        }

        void clear()
        {
            if constexpr (std::is_class_v<T>)
            {
                for(std::size_t idx = 0; idx < size_; ++idx)
                    deref(idx).~T();
            }

            size_ = 0;
        }

        [[nodiscard]] std::size_t size() const
        {
            return size_;
        }

        const T & operator[](const std::size_t idx) const
        {
            return deref(idx);
        }

    private:
        T & deref(const std::size_t idx)
        {
            assert(idx < size_);
            return *static_cast<T *>(static_cast<void *>(nodes_[idx].mem));
        }

        const T & deref(const std::size_t idx) const
        {
            return trait::remove_const(this)->deref(idx);
        }

    private:
        std::vector<Node> nodes_;
        std::size_t size_ = 0;
    };
}
