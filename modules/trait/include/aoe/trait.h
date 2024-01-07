//
// Created by yarten on 24-1-5.
//

#pragma once


namespace aoe::trait
{
    template <class T>
    constexpr decltype(auto) remove_const(const T& x)
    {
        return const_cast<T&>(x);
    }

    template <class T>
    constexpr decltype(auto) remove_const(const T* x)
    {
        return const_cast<T*>(x);
    }

    template <class T>
    constexpr decltype(auto) add_const(T& x)
    {
        return const_cast<const T&>(x);
    }

    template <class T>
    constexpr decltype(auto) add_const(T* x)
    {
        return const_cast<const T*>(x);
    }
}

namespace aoe::trait
{
    template<class ... TOps>
    struct impl : TOps ...
    {
        using TOps::operator()...;
    };
}
