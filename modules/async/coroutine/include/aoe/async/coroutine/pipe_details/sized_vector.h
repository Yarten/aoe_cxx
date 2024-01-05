//
// Created by yarten on 24-1-4.
//

#pragma once

#include <vector>
#include <cassert>
#include <atomic>


namespace aoe::async::coroutine::pipe_details
{
    template<class T>
    class SizedVector
    {
    public:
        class Element
        {
            enum class State : std::uint8_t
            {
                Init, Using, Destructed
            };
        public:
            template<class ... TArgs>
            T * construct(TArgs &&... args)
            {
                assert(state_.load(std::memory_order::relaxed) == State::Init);
                T * result = new(mem_) T(std::forward<TArgs>(args)...);
                state_.store(State::Using, std::memory_order::release);
                return result;
            }

            void destruct()
            {
                assert(state_.load(std::memory_order::relaxed) == State::Using);
                state_.store(State::Destructed, std::memory_order::release);
                static_cast<T*>(static_cast<void*>(mem_))->~T();
            }

            T * operator->()
            {
                assert(state_.load(std::memory_order::relaxed) == State::Using);
                return static_cast<T*>(static_cast<void*>(mem_));
            }

            [[nodiscard]] bool isUsing() const
            {
                return state_.load(std::memory_order::acquire) == State::Using;
            }

            [[nodiscard]] bool isDestructed() const
            {
                return state_.load(std::memory_order::acquire) == State::Destructed;
            }

            ~Element()
            {
                if (state_.load(std::memory_order::relaxed) == State::Using)
                    destruct();
            }

        private:
            // identify that this node is constructed, in-use, or destructed.
            std::atomic<State> state_ = State::Init;

            // memory of one T object
            std::byte mem_[sizeof(T)] {};
        };

    public:
        explicit SizedVector(std::size_t max_size)
            : elements_(max_size)
        {
        }

        Element & operator[](std::size_t idx)
        {
            return elements_[idx];
        }

        [[nodiscard]] std::size_t capacity() const
        {
            return elements_.size();
        }

    private:
        std::vector<Element> elements_;
    };
}
