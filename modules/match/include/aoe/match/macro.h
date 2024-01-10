//
// Created by yarten on 24-1-9.
//

#pragma once

#include <variant>
#include <aoe/macro.h>


/**
 * \brief Define a rust-like enum class
 * \code
 *   AOE_ENUM_CLASS(
 *     MyEnumName,
 *     (MyItemName1, Type1, Type2)
 *     (MyItemName2, Type3, Type4, Type5)
 *     (MyItemName3)
 *  );
 *
 *  MyEnumName x = MyEnumName::MyItemName1(Type1(), Type2());
 *
 *  // use match
 *  match(x) | aoe::trait::impl{
 *    MyEnumName::MyItemName1 | [](Type1, Type2) {},
 *    MyEnumName::MyItemName2 | [](Type3 &, Type4, auto) {},
 *    MyEnumName::MyItemName3 | []() {}
 *  };
 *
 *  // use if let
 *  ifLet(x) = MyEnumName::MyItemName1 | [](Type1, Type2) {};
 *
 *  // if let can be cascaded
 *  ifLet(x) |MyEnumName::MyItemName1| [](Type1, Type2) {}
 *  or
 *  ifLet(x) |MyEnumName::MyItemName2| [](auto, auto, auto) {}
 *  or
 *  aoe::trait::otherwise{
 *    [](){}
 *  };
 *  \endcode
 */
#define AOE_ENUM_CLASS(_name_, ...) AOE_ENUM_TEMPLATE_CLASS(_name_, (), __VA_ARGS__)

/**
 * \brief Define a rust-like enum class with template parameters
 * \code
 *   template<class T1, class T2>
 *   AOE_ENUM_TEMPLATE_CLASS(
 *      MyEnumName,
 *      (T1, T2),
 *      (MyItemName1, T1, int, T2)
 *      (MyItemName2, int, bool)
 *   );
 * \endcode
 */
#define AOE_ENUM_TEMPLATE_CLASS(_name_, _pack_templates_, ...)              \
    class _name_ : public ::aoe::match_details::EnumClass<                  \
        _name_ AOE_DETIALS_ENUM_TEMPLATE_OPT _pack_templates_               \
    >                                                                       \
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

// Declare an optional template parameter list
#define AOE_DETIALS_ENUM_TEMPLATE_OPT(...) \
    __VA_OPT__(<) __VA_ARGS__ __VA_OPT__(>)

// Declare an enum item for aoe enum class
#define AOE_DETAILS_ENUM_FIELD_UNPACK(_pack_params_) \
    AOE_DETAILS_ENUM_FIELD _pack_params_

#define AOE_DETAILS_ENUM_FIELD(_name_, ...) \
    private:                                \
        struct _name_ {};                   \
    public:                                 \
        static inline aoe::match_details::EnumField<_this_enum_class_type_, _name_ __VA_OPT__(,) __VA_ARGS__> _name_;

// Declare a possible variant type for enum
#define AOE_DETAILS_ENUM_TYPE_UNPACK(_pack_params_) \
    AOE_DETAILS_ENUM_TYPE _pack_params_

#define AOE_DETAILS_ENUM_TYPE(_name_, ...) \
    , typename decltype(_name_)::ValueType

namespace aoe::match_details
{
    template<class, class ... Ts>
    using VariantExceptFirstType = std::variant<Ts...>;
}
