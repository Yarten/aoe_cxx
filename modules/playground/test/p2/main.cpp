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

        if (c)
            c.resume();

        return 1;
    }

    MyWaitable(int _n)
        : n(_n)
    {
        std::cout << "MyWaitable() " << n << std::endl;
    }

    ~MyWaitable()
    {
        std::cout << "~MyWaitable() " << n << std::endl;
    }

    MyWaitable(int _n, std::coroutine_handle<> _c)
        : n(_n), c(_c)
    {
        std::cout << "MyWaitable() _c " << n << std::endl;
    }

    std::coroutine_handle<> c;
};

struct MyPromise;

struct MyCoroutine :
    std::coroutine_handle<MyPromise>
{
    using promise_type = MyPromise;

    MyCoroutine(std::coroutine_handle<MyPromise> && base)
        : std::coroutine_handle<MyPromise>(base)
    {
        std::cout << "MyCoroutine()" << std::endl;
    }

    ~MyCoroutine()
    {
        std::cout << "~MyCoroutine()" << std::endl;
    }
};

struct MyPromise
{
    MyCoroutine get_return_object()
    {
        // MyPromise 先于 MyCorountine 构造，并早于 MyCoroutine 析构。MyCoroutine 的 destroy() 可以删除本对象
        std::cout << "get return object" << std::endl;

        std::coroutine_handle<MyPromise> h[5];
        for(auto & i : h)
        {
            i = MyCoroutine::from_promise(*this);
            std::cout << i.address() << " " << &i.promise() << std::endl;
        }

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

    MyPromise()
    {
        std::cout << "MyPromise() " << this << std::endl;
    }

    ~MyPromise()
    {
        std::cout << "~MyPromise()" << this << std::endl;
    }

    MyWaitable await_transform(MyCoroutine c)
    {
        return {9, c};
    }

    MyWaitable await_transform(MyWaitable && w)
    {
        return w;
    }
};

MyCoroutine DoSomething(int n)
{
    std::cout << n << std::endl;
    int m = co_await MyWaitable(n);
    std::cout << "co_await " << m << std::endl;


    co_await DoSomething(n + 1);
    std::cout << "end of DoSomething() " << n << std::endl;
}


int main()
{
    std::cout << "main() begin" << std::endl;
    MyCoroutine h = DoSomething(5);

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
