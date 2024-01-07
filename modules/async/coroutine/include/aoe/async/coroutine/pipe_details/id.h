//
// Created by yarten on 24-1-1.
//

#pragma once

#include <atomic>


namespace aoe::async::coroutine::pipe_details
{
    template<class Tag>
    class Id
    {
        enum: std::uint32_t {
            INVALID_ID = static_cast<std::uint32_t>(-1),
            CLOSED_ID  = static_cast<std::uint32_t>(-2),
            MAX_ID     = static_cast<std::uint32_t>(-3)
        };
    public:
        Id() = default;

        Id(Id && other) noexcept
        {
            operator=(std::move(other));
        }

        Id & operator=(Id && other) noexcept
        {
            if (this == &other)
                return *this;

            num_ = other.num_;
            other.num_ = INVALID_ID;

            return *this;
        }

    public:
        [[nodiscard]] constexpr bool valid() const
        {
            return num_ != INVALID_ID and num_ != CLOSED_ID;
        }

        [[nodiscard]] constexpr bool closed() const
        {
            return num_ == CLOSED_ID;
        }

        [[nodiscard]] constexpr std::uint32_t num() const
        {
            return num_;
        }

        constexpr void close()
        {
            num_ = CLOSED_ID;
        }

        static Id fromNum(std::uint32_t num)
        {
            Id result;
            result.num_ = num;
            return result;
        }

    private:
        std::uint32_t num_ = INVALID_ID;

    public:
        class Creator
        {
        public:
            explicit Creator(const std::uint32_t max_count)
                : count_(max_count)
            {
                assert(max_count > 0 and max_count <= MAX_ID);
            }

            Id next()
            {
                std::uint32_t count = count_.load(std::memory_order::acquire);
                if (count == 0)
                    return {};

                while (not count_.compare_exchange_strong(
                    count,
                    count - 1,
                    std::memory_order::acquire, std::memory_order::relaxed
                    ))
                {
                    if (count == 0)
                        return {};
                }

                Id id;
                id.num_ = count - 1;
                return id;
            }

            [[nodiscard]] bool hasNext() const
            {
                return count_.load(std::memory_order::acquire) == 0;
            }

        private:
            std::atomic_uint32_t count_ = INVALID_ID;
        };
    };

    namespace details
    {
        struct SendTag;
        struct RecvTag;
    }

    using SendId = Id<details::SendTag>;
    using RecvId = Id<details::RecvTag>;
}
