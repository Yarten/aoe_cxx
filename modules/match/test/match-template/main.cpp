//
// Created by yarten on 24-1-10.
//

#include <aoe/match.h>
#include <iostream>


template<class TValue, class TError>
AOE_ENUM_TEMPLATE_CLASS(
    Result,
    (TValue, TError),
    (Value, TValue),
    (Error, TError)
);

namespace details
{
    template<class TValue>
    AOE_ENUM_TEMPLATE_CLASS(
        OptionImpl,
        (TValue),
        (Some, TValue),
        (None)
    );

    class OptionNoneImpl
    {
        template<class F>
        class Arm
        {
        public:
            explicit Arm(F && func)
                : func_(std::forward<F>(func))
            {
            }

            template<class T> requires(std::tuple_size_v<std::remove_reference_t<T>> == 0)
            auto operator()(T)
            {
                return func_();
            }

            constexpr static inline std::size_t ARM_INDEX = 1;

        private:
            F func_;
        };

    public:
        template<class F>
        auto operator|(F && func)
        {
            return Arm(std::forward<F>(func));
        }
    };
}

static inline details::OptionNoneImpl None;

template<class TValue>
class Option : public details::OptionImpl<TValue>
{
    using Super = details::OptionImpl<TValue>;
public:
    AOE_DECLARE_DEFAULT_COPY_MOVE(Option);

    Option(const Super & other)
        : Super(other)
    {
    }

    Option(Super && other) noexcept
        : Super(std::move(other))
    {
    }

    Option(decltype(None))
        : Super(Super::None())
    {
    }

    Option & operator=(const Super & other)
    {
        Super::operator=(other);
        return *this;
    }

    Option & operator=(Super && other) noexcept
    {
        Super::operator=(std::move(other));
        return *this;
    }

    Option & operator=(decltype(None))
    {
        Super::operator=(Super::None());
        return *this;
    }
};

namespace details
{
    class OptionSomeImpl
    {
        template<class F>
        class Arm
        {
        public:
            explicit Arm(F && func)
                : func_(std::forward<F>(func))
            {
            }

            template<class T> requires(std::tuple_size_v<std::remove_reference_t<T>> == 1)
            auto operator()(T && obj)
            {
                return func_(std::get<0>(std::forward<T>(obj)));
            }

            constexpr static inline std::size_t ARM_INDEX = 0;

        private:
            F func_;
        };

    public:
        template<class T>
        auto operator()(T && value) -> Option<T>
        {
            return Option<T>::Some(std::forward<T>(value));
        }

        template<class F>
        auto operator|(F && func)
        {
            return Arm(std::forward<F>(func));
        }
    };
}

static inline details::OptionSomeImpl Some;

int main()
{
    Result result = Result<int, float>::Value(1);

    Option y = Some(1);

    while (true)
    {
        bool is_none = false;

        match(y) | aoe::trait::impl{
            Some | [&](int x)
            {
                std::cout << x << " " << std::endl;
                y = None;
            },
            None | [&]()
            {
                std::cout << "none" << std::endl;
                is_none = true;
            }
        };

        if (is_none)
            break;
    }

    y = Some(2);

    ifLet(y) = Some | [](int & x)
    {
        std::cout << ++x << std::endl;
    };

    return 0;
}
