//
// Created by yarten on 24-1-9.
//

#pragma once

#include <cstddef>
#include <aoe/trait.h>
#include "./enum_value.h"


namespace aoe::match_details
{
    template<class TDerived>
    class EnumClass
    {
        class Match
        {
        public:
            template<class TArms>
            auto operator|(TArms && arms)
            {
                offsetof(EnumClass, match);
            }
        };
    public:
        EnumClass() = default;
        EnumClass(const EnumClass &) = default;
        EnumClass(EnumClass &&) = default;
        EnumClass & operator=(const EnumClass &) = default;
        EnumClass & operator=(EnumClass &&) = default;

        template<class EValueType>
        EnumClass(EValueType && value)
        {
            derived().data_ = std::forward<EValueType>(value);
        }

        template<class EValueType>
        EnumClass & operator=(EValueType && value)
        {
            derived().data_ = std::forward<EValueType>(value);
            return *this;
        }

    private:
        TDerived & derived()
        {
            return *static_cast<TDerived*>(this);
        }

        const TDerived & derived() const
        {
            return trait::remove_const(this)->derived();
        }

    public:
        Match match;
    };
}
