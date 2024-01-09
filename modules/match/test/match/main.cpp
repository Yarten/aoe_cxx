//
// Created by yarten on 24-1-9.
//

#include <variant>
#include <tuple>
#include <type_traits>

#include <aoe/match.h>
#include <aoe/macro.h>


class X
{
public:
    struct A : aoe::match_details::EnumField<A, int, double>
    {
    } static inline A;

    std::variant<decltype(A)> x;
};


AOE_ENUM_CLASS(XX, (A, int, double), (B, bool));

template<class TDerived>
class BaseClass
{
public:
    void f()
    {
        using Type = typename TDerived::MyType;

        // static_cast<TDerived*>(this)->g();
    }
};

class DerivedClass : public BaseClass<DerivedClass>
{
public:
    using MyType = int;

private:
    void g() {}
};


int main()
{
    XX a = XX::A(1, 2);

    a = XX::B(true);

    int aa = 1;
    double bb = 2;

    (XX::A | [](const int &, const double &)
    {

    })(aa, bb);

    XX x;

    DerivedClass y;
    y.f();

    auto z = XX::A(1, 2);
}
