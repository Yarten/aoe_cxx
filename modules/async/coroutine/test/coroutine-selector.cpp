//
// Created by yarten on 24-1-3.
//

#include <iostream>
#include <aoe/async/coroutine/selector.h>


class FakeAwaiter
{
public:
    explicit FakeAwaiter(bool is_ready, int n)
        : is_ready_(is_ready), n_(n)
    {
        std::cout << "FakeAwaiter() " << n_ << std::endl;
    }

    ~FakeAwaiter()
    {
        std::cout << "~FakeAwaiter() " << n_ << std::endl;
    }

    bool await_resume()
    {
        std::cout << "await_resume() " << n_ << std::endl;
        return true;
    }

    void await_abort()
    {
        std::cout << "await_abort() " << n_ << std::endl;
    }

    bool isReady() const
    {
        return is_ready_;
    }

private:
    bool is_ready_ = false;
    int  n_ = 0;
};


int main()
{
    int n = 0;

    aoe::async::coroutine::Selector x{
        FakeAwaiter(false, ++n),
        FakeAwaiter(true, ++n),
        FakeAwaiter(true, ++n)
    };
    x.onResume();

    return 0;
}
