//
// Created by yarten on 24-1-1.
//

#include <variant>
#include <atomic>
#include <coroutine>
#include <vector>
#include <string>
#include <iostream>


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


int main()
{
    g1("abc");
    g2("edf");
    return 0;
}
