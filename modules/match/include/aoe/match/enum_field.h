//
// Created by yarten on 24-1-9.
//

#pragma once

#include "./enum_value.h"
#include "./enum_class.h"


namespace aoe::match_details
{
    template<class TEnumClass, class Tag, class ... Ts>
    class EnumField
    {
    public:
        using ValueType = EnumValue<Tag, Ts...>;

    private:
        template<class F, std::size_t ... I>
        class Arm
        {
        public:
            Arm(F && func, std::index_sequence<I...>)
                : func_(std::forward<F>(func))
            {
            }

            using ArmType = ValueType;

        public:
            auto operator()(const ValueType & value) const
            {
                return func_(std::get<I>(value)...);
            }

            auto operator()(ValueType & value) const
            {
                return func_(std::get<I>(value)...);
            }

            auto operator()(ValueType && value) const
            {
                return func_(std::get<I>(std::move(value))...);
            }

        private:
            F func_;
        };

    public:
        TEnumClass operator()(Ts &&... args) const
        {
            return TEnumClass(ValueType(std::forward<Ts>(args)...), BuildByEnumValue());
        }

        template<class F>
        auto operator|(F && func) const
        {
            return Arm(std::forward<F>(func), std::make_index_sequence<sizeof...(Ts)>());
        }
    };
}
