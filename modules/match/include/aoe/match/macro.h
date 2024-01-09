//
// Created by yarten on 24-1-9.
//

#pragma once

#include <variant>
#include <aoe/macro.h>




#define AOE_ENUM_CLASS(_name_, ...)                               \
    class _name_                                                  \
    {                                                             \
    public:                                                       \
        AOE_INVOKE(AOE_DETAILS_ENUM_FIELD_UNPACK, __VA_ARGS__)    \
    private:                                                      \
        ::aoe::match_details::VariantExceptFirstType<             \
            std::nullptr_t                                        \
            AOE_INVOKE(AOE_DETAILS_ENUM_TYPE_UNPACK, __VA_ARGS__) \
        > data_;                                                  \
    }

// Declare an enum item for aoe enum class
#define AOE_DETAILS_ENUM_FIELD_UNPACK(_pack_params_) \
    AOE_DETAILS_ENUM_FIELD _pack_params_

#define AOE_DETAILS_ENUM_FIELD(_name_, ...) \
    private:                                \
        struct _name_ {};                   \
    public:                                 \
        static inline aoe::match_details::EnumField<_name_, __VA_ARGS__> _name_;

// Declare a possible variant type for enum
#define AOE_DETAILS_ENUM_TYPE_UNPACK(_pack_params_) \
    AOE_DETAILS_ENUM_TYPE _pack_params_

#define AOE_DETAILS_ENUM_TYPE(_name_, ...) \
    , decltype(_name_)

namespace aoe::match_details
{
    template<class, class ... Ts>
    using VariantExceptFirstType = std::variant<Ts...>;
}
