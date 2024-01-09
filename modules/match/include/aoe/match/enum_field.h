//
// Created by yarten on 24-1-9.
//

#pragma once

#include <tuple>


namespace aoe::match_details
{
    template<class Tag, class ... Ts>
    class EnumField :
        public std::tuple<Ts...>
    {
    public:
        using std::tuple<Ts...>::tuple;

    public:
        EnumField operator()(Ts &&... args) const
        {
            return EnumField(std::forward<Ts>(args)...);
        }
    };
}

namespace std
{
    template<class Tag, class ... Ts>
    struct tuple_size<aoe::match_details::EnumField<Tag, Ts...>>
        : integral_constant<size_t, sizeof...(Ts)>
    {
    };

    template<size_t I, class Tag, class ... Ts>
    struct tuple_element<I, aoe::match_details::EnumField<Tag, Ts...>>
    {
        using type = tuple_element_t<I, std::tuple<Ts...>>;
    };
}
