//
// Created by yarten on 23-12-26.
//

#include <aoe/async/coroutine.h>
#include <iostream>
#include <vector>


aoe::async::Yield<int> getValue()
{
    for(int i = 0; i < 10; ++i)
        co_yield i;
}

aoe::async::Go<double> doSomethingForValue()
{
    co_return 3.0;
}

aoe::async::Go<> printValue()
{
    auto h = getValue();

    for (int n = 0;
        co_await(
            h >> n
            & [](){ std::cout << "cb "; }
            | [](){ std::cout << "END" << std::endl; }
        );
    )
    {
        std::cout << n << std::endl;
    }

    std::cout << "final " << (co_await doSomethingForValue()) << std::endl;
}

int main()
{
    printValue();
    return 0;
}
