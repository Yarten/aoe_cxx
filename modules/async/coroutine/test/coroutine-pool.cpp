//
// Created by yarten on 24-1-13.
//

#include <iostream>
#include <aoe/async/coroutine.h>

aoe::async::Go<> func(int a, int b)
{
    std::cout << "func() " << a << " " << b << std::endl;
    co_return;
}


int main()
{
    for(int i = 0; i < 10000; ++i)
    {
        aoe::async::go(func, i, i + 1);
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}
