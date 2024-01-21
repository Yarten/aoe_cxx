//
// Created by yarten on 24-1-17.
//

#include <aoe/async/coroutine.h>

aoe::async::Go<> sendFunc(aoe::async::Pipe<int> ch)
{
    for(int i = 1; i <= 10; ++i)
    {
        std::cout << "before send " << i << std::endl;
        co_await(ch << i);
        std::cout << "after send " << i << std::endl;

        if (i % 2 == 0)
        {
            std::cout << ">>> send sleep >>>" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::cout << "sendFunc() exit" << std::endl;
}

aoe::async::Go<> recvFunc(aoe::async::Pipe<int> ch)
{
    for(int i = 1; i <= 10; ++i)
    {
        std::cout << "before recv " << i << std::endl;
        co_await(ch >> [](int n)
        {
            std::cout << "after recv " << n << std::endl;
        });

        if (i % 2 != 0)
        {
            std::cout << "<<< recv sleep <<<" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::cout << "recvFunc() exit" << std::endl;
}

int main()
{
    auto ch = aoe::async::go.makePipe<int>(0ul, 1ul, 1ul, aoe::async::coroutine::BroadcastNone());

    aoe::async::go(recvFunc, ch.deviler());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    aoe::async::go(sendFunc, ch.deviler());

    std::this_thread::sleep_for(std::chrono::seconds(20));
    return 0;
}
