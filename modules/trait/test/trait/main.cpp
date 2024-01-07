//
// Created by yarten on 24-1-7.
//

#include <iostream>
#include <variant>
#include <string>
#include <aoe/trait.h>


int main()
{
    std::variant<int, double, bool, std::string> v;

    auto visit = [&]()
    {
        std::visit(
            aoe::trait::impl {
                [](const int x)
                {
                    std::cout << "int " << x << std::endl;
                },
                []<class T>(const T & y)
                {
                    std::cout << typeid(decltype(y)).name() << " " << y << std::endl;
                }
            }, v
        );
    };

    v.emplace<int>(1);
    visit();

    v.emplace<double>(2.0);
    visit();

    v.emplace<bool>(true);
    visit();

    v.emplace<std::string>("abc");
    visit();

    class A
    {
    public:
        using Type = int;
    };

    f(
        [](int x, int b)
        {
            return A();
        },
        1, 2
    );


    return 0;
}
