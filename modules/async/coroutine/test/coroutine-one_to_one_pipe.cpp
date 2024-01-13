//
// Created by yarten on 24-1-13.
//

#include <aoe/async/coroutine.h>


aoe::async::Go<> sendFunc(aoe::async::Pipe<int> ch)
{
    int n = 1;

    std::cout << "before send " << n << " " << std::this_thread::get_id() << std::endl;
    co_await (ch << n);
    std::cout << "after send " << n << std::endl;
}

aoe::async::Go<> recvFunc(aoe::async::Pipe<int> ch)
{
    int n = 0;

    std::cout << "wait for n ... " << std::this_thread::get_id() << std::endl;
    co_await (ch >> n);
    std::cout << "recv " << n << std::endl;
}


int main()
{
    auto ch = aoe::async::go.makePipe<int>(5ul, 1ul, 1ul, aoe::async::coroutine::BroadcastNone());

    aoe::async::go(recvFunc, ch.deviler());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    aoe::async::go(sendFunc, ch.deviler());

    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 0;
}
