//
// Created by yarten on 24-1-13.
//

#include <string>
#include <aoe/async/coroutine.h>


aoe::async::Go<> recvFunc(
    aoe::async::Pipe<int> ch1,
    aoe::async::Pipe<double> ch2,
    aoe::async::Pipe<std::string> ch3
)
{
    for (int i = 0; i < 10; ++i)
    {
        int n = 0;
        double d = 0;

        std::cout << "recvFunc() " << std::this_thread::get_id() << std::endl;

        co_await aoe::async::select{
            ch1 >> n & [&]()
            {
                std::cout << "recv int " << n << std::endl;
            },
            ch2 >> d & [&]()
            {
                std::cout << "recv double " << d << std::endl;
            },
            ch3 >> [](std::string s)
            {
                std::cout << "recv string " << s << std::endl;
            }
        };
    }
}

aoe::async::Go<> sendIntFunc(aoe::async::Pipe<int> ch)
{
    for (int i = 0; i < 3; ++i)
    {
        std::cout << "sendIntFunc() " << std::this_thread::get_id() << std::endl;
        co_await (ch << (i + 1));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
}

aoe::async::Go<> sendDoubleFunc(aoe::async::Pipe<double> ch)
{
    for (int i = 0; i < 2; ++i)
    {
        std::cout << "sendDoubleFunc() " << std::this_thread::get_id() << std::endl;
        co_await (ch << static_cast<double>(i + 3));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
}

aoe::async::Go<> sendStringFunc(aoe::async::Pipe<std::string> ch)
{
    for (int i = 0; i < 5; ++i)
    {
        std::cout << "sendStringFunc() " << std::this_thread::get_id() << std::endl;
        co_await (ch << std::to_string(i + 5));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
}

int main()
{
    auto ch_i = aoe::async::go.makePipe<int>(2ul, 1ul, 1ul, aoe::async::coroutine::BroadcastNone());
    auto ch_d = aoe::async::go.makePipe<double>(2ul, 1ul, 1ul, aoe::async::coroutine::BroadcastNone());
    auto ch_s = aoe::async::go.makePipe<std::string>(2ul, 1ul, 1ul, aoe::async::coroutine::BroadcastNone());

    aoe::async::go(recvFunc, ch_i.deviler(), ch_d.deviler(), ch_s.deviler());
    aoe::async::go(sendIntFunc, std::move(ch_i));
    aoe::async::go(sendDoubleFunc, std::move(ch_d));
    aoe::async::go(sendStringFunc, std::move(ch_s));

    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
