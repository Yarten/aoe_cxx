//
// Created by yarten on 24-1-9.
//

#pragma once

#include <tuple>


namespace aoe::match_details
{
    template<class, class ... Ts>
    class EnumValue :
        public std::tuple<Ts...>
    {
    public:
        using std::tuple<Ts...>::tuple;
    };
}

namespace std
{
    template<class Tag, class ... Ts>
    struct tuple_size<aoe::match_details::EnumValue<Tag, Ts...>>
        : integral_constant<size_t, sizeof...(Ts)>
    {
    };

    template<size_t I, class Tag, class ... Ts>
    struct tuple_element<I, aoe::match_details::EnumValue<Tag, Ts...>>
    {
        using type = tuple_element_t<I, std::tuple<Ts...>>;
    };
}
