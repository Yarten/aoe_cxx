//
// Created by yarten on 24-1-5.
//

#pragma once

#include <type_traits>


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

namespace aoe::trait
{
    namespace details
    {
        template<class F>
        concept VoidFunctionTrait = requires(F && func)
        {
            { func() } -> std::same_as<void>;
        };
    }

    template<class TVoidOp> requires(details::VoidFunctionTrait<TVoidOp>)
    struct otherwise : TVoidOp
    {
        using TVoidOp::operator();
    };

    template<class TVoidOp>
    constexpr void operator||(const bool con, otherwise<TVoidOp> && ops)
    {
        if (not con)
            ops();
    }
}
