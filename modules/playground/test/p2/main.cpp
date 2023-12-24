//
// Created by yarten on 23-12-23.
//

#include <iostream>
#include <coroutine>

struct MyWaitable
{
    int n = 0;

    bool await_ready() noexcept
    {
        std::cout << "await ready " << n << std::endl;
        return n == 2;
    }

    void await_suspend(std::coroutine_handle<> h) noexcept
    {
        std::cout << "await suspend " << n << std::endl;
    }

    int await_resume() noexcept
    {
        std::cout << "await resume " << n << std::endl;
        return 1;
    }
};

struct MyPromise;

struct MyCoroutine :
    std::coroutine_handle<MyPromise>
{
    using promise_type = MyPromise;
};

struct MyPromise
{
    MyCoroutine get_return_object()
    {
        std::cout << "get return object" << std::endl;

        // from_promise() 返回了 std::corouting_handle<>
        return {MyCoroutine::from_promise(*this)};
    }

    MyWaitable initial_suspend() noexcept
    {
        std::cout << "initial suspend" << std::endl;
        return {1};
    }

    MyWaitable final_suspend() noexcept
    {
        std::cout << "final suspend" << std::endl;
        return {2};
    }

    void return_void()
    {
        std::cout << "return void" << std::endl;
    }

    void unhandled_exception()
    {
        std::cout << "unhandled exception" << std::endl;
    }
};

MyCoroutine DoSomehing(int n)
{
    std::cout << n << std::endl;
    int m = co_await MyWaitable();
    std::cout << "co_await " << m << std::endl;
    co_return;
}


int main()
{
    std::cout << "main() begin" << std::endl;
    MyCoroutine h = DoSomehing(1);

    std::cout << "After create h, before h.resume()" << std::endl;
    h.resume();

    std::cout << "after h.resume()" << std::endl;

    if (not h.done())
    {
        std::cout << "no done yet" << std::endl;
        h.resume();
    }

    if (h.done())
    {
        std::cout << "now destory()" << std::endl;
        h.destroy();
    }
    else
    {
        std::cout << "auto destory()" << std::endl;
    }


    return 0;
}
