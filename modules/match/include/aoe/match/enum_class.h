//
// Created by yarten on 24-1-9.
//

#pragma once

#include <variant>
#include <aoe/trait.h>
#include <aoe/macro.h>

#include <iostream>


namespace aoe::match_details
{
    struct BuildByEnumValue {};

    template<class T, class TVariant>
    struct GetIndex;

    template<class>
    struct GetIndexTag {};

    template<class T, class ... Ts>
    struct GetIndex<T, std::variant<Ts...>> :
        std::integral_constant<
            std::size_t,
            std::variant<GetIndexTag<Ts>...>(GetIndexTag<T>()).index()
        >
    {
    };

    template<class TDerived>
    class EnumClass
    {
        template<class E>
        class Match
        {
        public:
            explicit Match(E self)
                : self_(std::forward<E>(self))
            {
            }

        public:
            template<class TArms>
            auto operator|(TArms && arms)
            {
                if constexpr (std::is_rvalue_reference_v<E>)
                {
                    return std::visit(std::forward<TArms>(arms), std::move(self_.data()));
                }
                else
                {
                    return std::visit(std::forward<TArms>(arms), self_.data());
                }
            }

        private:
            E self_;
        };

        template<class E>
        class IfLet
        {
            template<class TEnumField>
            class Some
            {
            public:
                Some(IfLet && self, const TEnumField & field)
                    : self_(std::move(self)), field_(field)
                {
                }

                template<class F>
                bool operator|(F && func) const
                {
                    return self_.operator=(field_ | std::forward<F>(func));
                }

            private:
                IfLet && self_;
                const TEnumField & field_;
            };
        public:
            explicit IfLet(E self)
                : self_(std::forward<E>(self))
            {
            }

        public:
            template<class TArm>
            bool operator=(TArm && arm)
            {
                auto & data = self_.data();

                using VariantType = std::remove_reference_t<decltype(data)>;
                constexpr std::size_t ARM_INDEX = GetIndex<typename TArm::ArmType, VariantType>::value;

                if (data.index() != ARM_INDEX)
                    return false;

                if constexpr (std::is_rvalue_reference_v<E>)
                {
                    static_assert(std::is_void_v<decltype(arm(std::get<ARM_INDEX>(std::move(data))))>);
                    arm(std::get<ARM_INDEX>(std::move(data)));
                }
                else
                {
                    static_assert(std::is_void_v<decltype(arm(std::get<ARM_INDEX>(data)))>);
                    arm(std::get<ARM_INDEX>(data));
                }

                return true;
            }

            template<class TEnumField>
            auto operator|(const TEnumField & field) &&
            {
                return Some<TEnumField>(std::move(*this), field);
            }

        private:
            E self_;
        };

    private:
        TDerived & derived() &
        {
            return *static_cast<TDerived*>(this);
        }

        TDerived && derived() &&
        {
            return std::move(derived());
        }

        const TDerived & derived() const &
        {
            return trait::remove_const(this)->derived();
        }

        auto & data() &
        {
            return derived().data_;
        }

        auto && data() &&
        {
            return std::move(derived().data_);
        }

        const auto & data() const &
        {
            return derived().data_;
        }

    public:
        friend auto match(const TDerived & obj)
        {
            return Match<const TDerived &>(obj);
        }

        friend auto match(TDerived & obj)
        {
            return Match<TDerived &>(obj);
        }

        friend auto match(TDerived && obj)
        {
            return Match<TDerived &&>(std::move(obj));
        }

        friend auto ifLet(const TDerived & obj)
        {
            return IfLet<const TDerived &>(obj);
        }

        friend auto ifLet(TDerived & obj)
        {
            return IfLet<TDerived &>(obj);
        }

        friend auto ifLet(TDerived && obj)
        {
            return IfLet<TDerived &&>(std::move(obj));
        }
    };
}
