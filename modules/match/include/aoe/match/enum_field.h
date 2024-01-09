//
// Created by yarten on 24-1-9.
//

#pragma once

#include "./enum_value.h"


namespace aoe::match_details
{
    template<class TEnumClass, class Tag, class ... Ts>
    class EnumField
    {
        template<class F, std::size_t ... I>
        class Arm
        {
        public:
            Arm(F && func, std::index_sequence<I...>)
                : func_(std::forward<F>(func))
            {
            }

        public:
            auto operator()(const Ts &... args) const
            {
                return func_(args...);
            }

            auto operator()(Ts &... args) const
            {
                return func_(args...);
            }

            auto operator()(Ts &&... args) const
            {
                return func_(std::move(args)...);
            }

        private:
            F func_;
        };
    public:
        using ValueType = EnumValue<Tag, Ts...>;

    public:
        TEnumClass operator()(Ts &&... args) const
        {
            return TEnumClass(ValueType(std::forward<Ts>(args)...));
        }

        template<class F>
        auto operator|(F && func)
        {
            return Arm(std::forward<F>(func), std::make_index_sequence<sizeof...(Ts)>());
        }
    };
}
