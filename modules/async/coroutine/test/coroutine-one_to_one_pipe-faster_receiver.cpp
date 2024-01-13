//
// Created by yarten on 24-1-13.
//

#include <aoe/async/coroutine.h>


constexpr int COUNT = 20;

aoe::async::Go<> sendFunc(aoe::async::Pipe<int> ch)
{
    for(int n = 1; n <= COUNT; ++n)
    {
        std::cout << "before send " << n << std::endl;
        co_await (ch << n);
        std::cout << "after send " << n << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

aoe::async::Go<> recvFunc(aoe::async::Pipe<int> ch)
{
    for(int n = 1; n <= COUNT; ++n)
    {
        std::cout << "wait for n ..." << std::endl;
        co_await (ch >> n);
        std::cout << "recv " << n << std::endl;
    }
}

int main()
{
    auto ch = aoe::async::go.makePipe<int>(5ul, 1ul, 1ul, aoe::async::coroutine::BroadcastNone());

    aoe::async::go(sendFunc, ch.deviler());
    std::this_thread::sleep_for(std::chrono::seconds(5));
    aoe::async::go(recvFunc, ch.deviler());

    std::this_thread::sleep_for(std::chrono::seconds(COUNT + 5));
    return 0;
}
