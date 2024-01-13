//
// Created by yarten on 24-1-1.
//

#include <variant>
#include <atomic>
#include <coroutine>
#include <vector>
#include <string>
#include <iostream>
#include <optional>


template<class T>
void f(T && x)
{
    std::cout
        << std::is_move_assignable_v<T> << " "
        << typeid(T).name() << " " << x << std::endl;
}

void g1(const std::string & x)
{
    f(x);
}

void g2(std::string && y)
{
    f(std::move(y));
}

struct A
{
    int & x;
};

A f(int & x)
{
    return {x};
}


int main()
{
    g1("abc");
    g2("edf");

    int n = 1;
    auto a = f(n);

    std::variant<int, A> b;
    b.emplace<A>(f(n));

    return 0;
}
