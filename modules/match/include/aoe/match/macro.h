//
// Created by yarten on 24-1-9.
//

#pragma once

#include <variant>
#include <aoe/macro.h>


#define AOE_ENUM_CLASS(_name_, ...)                                         \
    class _name_ : public ::aoe::match_details::EnumClass<_name_>           \
    {                                                                       \
        using _this_enum_class_type_ = _name_;                              \
    public:                                                                 \
        _name_() = delete;                                                  \
        AOE_DECLARE_DEFAULT_COPY_MOVE(_name_);                              \
                                                                            \
        template<class EValueType>                                          \
        _name_(EValueType && value, ::aoe::match_details::BuildByEnumValue) \
            : data_(std::forward<EValueType>(value))                        \
        {                                                                   \
        }                                                                   \
                                                                            \
        AOE_INVOKE(AOE_DETAILS_ENUM_FIELD_UNPACK, __VA_ARGS__)              \
    private:                                                                \
        ::aoe::match_details::VariantExceptFirstType<                       \
            std::nullptr_t                                                  \
            AOE_INVOKE(AOE_DETAILS_ENUM_TYPE_UNPACK, __VA_ARGS__)           \
        > data_;                                                            \
                                                                            \
        friend class ::aoe::match_details::EnumClass<_name_>;               \
    }

// Declare an enum item for aoe enum class
#define AOE_DETAILS_ENUM_FIELD_UNPACK(_pack_params_) \
    AOE_DETAILS_ENUM_FIELD _pack_params_

#define AOE_DETAILS_ENUM_FIELD(_name_, ...) \
    private:                                \
        struct _name_ {};                   \
    public:                                 \
        static inline aoe::match_details::EnumField<_this_enum_class_type_, _name_, __VA_ARGS__> _name_;

// Declare a possible variant type for enum
#define AOE_DETAILS_ENUM_TYPE_UNPACK(_pack_params_) \
    AOE_DETAILS_ENUM_TYPE _pack_params_

#define AOE_DETAILS_ENUM_TYPE(_name_, ...) \
    , decltype(_name_)::ValueType

namespace aoe::match_details
{
    template<class, class ... Ts>
    using VariantExceptFirstType = std::variant<Ts...>;
}
