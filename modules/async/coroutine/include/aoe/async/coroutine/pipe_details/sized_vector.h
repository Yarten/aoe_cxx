//
// Created by yarten on 24-1-4.
//

#pragma once

#include <vector>
#include <cassert>


namespace aoe::async::coroutine::pipe_details
{
    template<class T>
    class SizedVector
    {
    public:
        class Element
        {
            enum class State
            {
                Init, Using, Destructed
            };
        public:
            template<class ... TArgs>
            T * construct(TArgs &&... args)
            {
                assert(state_ == State::Init);
                state_ = State::Using;
                return new(mem_) T(std::forward<TArgs>(args)...);
            }

            void destruct()
            {
                assert(state_ == State::Using);
                state_ = State::Destructed;
                static_cast<T*>(static_cast<void*>(mem_))->~T();
            }

            T * operator->()
            {
                assert(state_ == State::Using);
                return static_cast<T*>(static_cast<void*>(mem_));
            }

            [[nodiscard]] bool isDestructed() const
            {
                return state_ == State::Destructed;
            }

            ~Element()
            {
                if (state_ == State::Using)
                    destruct();
            }

        private:
            // identify that this node is constructed, in-use, or destructed.
            State state_ = State::Init;

            // memory of one T object
            std::byte mem_[sizeof(T)] {};
        };

    public:
        explicit SizedVector(std::size_t size)
            : elements_(size)
        {
        }

        Element & operator[](std::size_t idx)
        {
            return elements_[idx];
        }

    private:
        std::vector<Element> elements_;
    };
}
