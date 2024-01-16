//
// Created by yarten on 24-1-15.
//

#include <string>
#include <aoe/async/coroutine.h>


aoe::async::Go<> sendFunc(
    aoe::async::Pipe<int> ch1,
    aoe::async::Pipe<double> ch2,
    aoe::async::Pipe<std::string> ch3
)
{
    for (int i = 0; i < 10; ++i)
    {
        std::cout << "sendFunc() " << std::this_thread::get_id() << std::endl;

        int n = i;
        double d = i;
        std::string s = std::to_string(i);

        co_await aoe::async::select{
            ch1 << n & [&]()
            {
                std::cout << "send int " << n << std::endl;
            },
            ch2 << d & [&]()
            {
                std::cout << "send double " << d << std::endl;
            },
            ch3 << s & [&]()
            {
                std::cout << "send string " << s << std::endl;
            }
        };
    }
}

aoe::async::Go<> recvIntFunc(aoe::async::Pipe<int> ch)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "recvIntFunc() " << std::this_thread::get_id() << std::endl;

        const bool status = co_await (
            ch >> [](int n)
            {
                std::cout << "recv int " << n << std::endl;
            }
        );

        if (not status)
            break;
    }
}

aoe::async::Go<> recvDoubleFunc(aoe::async::Pipe<double> ch)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        std::cout << "recvDoubleFunc() " << std::this_thread::get_id() << std::endl;

        const bool status = co_await (
            ch >> [](double n)
            {
                std::cout << "recv double " << n << std::endl;
            }
        );

        if (not status)
            break;
    }
}

aoe::async::Go<> recvStringFunc(aoe::async::Pipe<std::string> ch)
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "recvStringFunc() " << std::this_thread::get_id() << std::endl;

        const bool status = co_await(
            ch >> [](std::string s)
            {
                std::cout << "recv string " << s << std::endl;
            }
        );

        if (not status)
            break;
    }
}

int main()
{
    auto ch_i = aoe::async::go.makePipe<int>(2ul, 1ul, 1ul, aoe::async::coroutine::BroadcastNone());
    auto ch_d = aoe::async::go.makePipe<double>(2ul, 1ul, 1ul, aoe::async::coroutine::BroadcastNone());
    auto ch_s = aoe::async::go.makePipe<std::string>(2ul, 1ul, 1ul, aoe::async::coroutine::BroadcastNone());

    aoe::async::go(sendFunc, ch_i.deviler(), ch_d.deviler(), ch_s.deviler());
    aoe::async::go(recvIntFunc, std::move(ch_i));
    aoe::async::go(recvDoubleFunc, std::move(ch_d));
    aoe::async::go(recvStringFunc, std::move(ch_s));

    std::this_thread::sleep_for(std::chrono::seconds(30));
    return 0;
}
