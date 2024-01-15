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
        std::cout << "sendFunc()" << std::this_thread::get_id() << std::endl;

        int n = i + 1;
        double d = (i + 1) * 0.3;
        std::string s = std::to_string(4 * i);

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

}
