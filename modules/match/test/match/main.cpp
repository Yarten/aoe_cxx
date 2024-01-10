//
// Created by yarten on 24-1-9.
//

#include <iostream>
#include <string>

#include <aoe/match.h>

struct MyClass
{
    MyClass()
    {
        std::cout << "MyClass()" << std::endl;
    }

    MyClass(const MyClass&)
    {
        std::cout << "MyClass(copy)" << std::endl;
    }

    ~MyClass()
    {
        std::cout << "~MyClass()" << std::endl;
    }
};

AOE_ENUM_CLASS(XX, (A, int, double), (B, bool, MyClass), (C, std::string, int, std::string));

int main()
{
    XX a = XX::A(1, 2);
    XX b = XX::B(true, MyClass());
    XX c(a);

    // default construct is deleted
    // XX x;

    a = b;

    // match
    match(a) | aoe::trait::impl{
        XX::A | [](int& x, const double y)
        {
            ++x;
            std::cout << x << " " << y << std::endl;
        },
        XX::B | [](const bool z, MyClass&)
        {
            std::cout << "match lvalue: " << z << std::endl;
        },
        XX::C | [](const std::string& x, int, const std::string&)
        {
        }
    };

    const double r = match(XX::C("abc", 30, "edf")) | aoe::trait::impl{
        XX::A | [](int, double)
        {
            return 1.1;
        },
        XX::B | [](bool, auto)
        {
            // return true; // all arms should have the same return type
            return 2.2;
        },
        XX::C | [](std::string&& a, const int b, std::string&& c)
        {
            std::cout << "match rvalue: " << a << " " << b << " " << c << std::endl;
            return 3.3;
        }
    };
    std::cout << "result is " << r << std::endl;

    // if let
    ifLet(c) = XX::B | [](auto, auto)
    {
        std::cout << "c is not XX::B !" << std::endl;
    };

    ifLet(c) = XX::A | [](int& x, const double y)
    {
        ++x;
        std::cout << "if let lvalue: " << x << " " << y << std::endl;
    };

    ifLet(XX::C("qqq", 40, "xxx")) = XX::C | [](auto&& x, auto&& y, auto&& z)
    {
        std::cout << "if let rvalue " << x << " " << y << " " << z << std::endl;
    };

    // if let with boolean condition cascading judgment
    ifLet(c) > XX::B | [](auto, auto)
    {
        std::cout << "c is not XX::B !" << std::endl;
    }
    or ifLet(c) > XX::A | [](int& x, const double y)
    {
        ++x;
        std::cout << "if let use >: " << x << " " << y << std::endl;
    };

    // use trait::otherwise to handle the 'else' case
    ifLet(c) > XX::B | [](auto, auto)
    {
        std::cout << "c is not XX::B ! Somehing wrong !" << std::endl;
    }
    or aoe::trait::otherwise {
        []()
        {
            std::cout << "c is not XX::B. Everything good !" << std::endl;
        }
    };

    return 0;
}
