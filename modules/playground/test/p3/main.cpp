//
// Created by yarten on 23-12-25.
//

#include <coroutine>
#include <iostream>


class MyCoroutine
{
public:
    class MyPromise
    {
    public:
        class MyWaitable
        {
        public:
            MyWaitable(std::coroutine_handle<MyPromise> h)
                : h_(h)
            {
                std::cout << "MyWaitable() " << this << std::endl;
            }

            ~MyWaitable()
            {
                std::cout << "~MyWaitable() " << this << std::endl;
            }

        public:
            bool await_ready() noexcept
            {
                return false;
            }

            void await_suspend(std::coroutine_handle<MyPromise> h) noexcept
            {
                std::cout << h_.address() << " " << h.address() << std::endl;
            }

            double await_resume() noexcept
            {
                // 若 coroutine 已经结束，即使是在 final point 挂起，都不可以调用 resume
                if (not h_.promise().is_return)
                    h_.resume();

                return h_.promise().value;
            }

        private:
            std::coroutine_handle<MyPromise> h_;
        };

    public:
        MyPromise()
        {
            std::cout << "MyPromise() " << this << std::endl;
        }

        ~MyPromise()
        {
            std::cout << "~MyPromise() " << this << std::endl;
        }

    public:
        std::coroutine_handle<MyPromise> get_return_object()
        {
            auto r  = std::coroutine_handle<promise_type>::from_promise(*this);
            std::cout << "build coroutine_handle " << r.address() << " " << &(r.promise()) << std::endl;
            return r;
        }

        std::suspend_always initial_suspend() noexcept
        {
            std::cout << "init point begin " << this << std::endl;
            return {};
        }

        std::suspend_always final_suspend() noexcept
        {
            std::cout << "reach final point " << this << std::endl;
            return {};
        }

        void unhandled_exception()
        {
        }

        void return_value(int n)
        {
            value = n * 2.5;
            std::cout << "return value " << n << " set value " << value << std::endl;
            is_return = true;
        }

        void return_value(std::string x)
        {
            std::cout << "return void" << std::endl;
            is_return = true;
        }

        // 存在 return_value() 的时候，不能定义 return_void()
        // 但却可以定义多个 return_value() !!
        // void return_void()
        // {
        //
        // }

        MyWaitable await_transform(MyCoroutine h)
        {
            return {h.h_};
        }

        double value = 0.0;

        bool is_return = false;
    };

    using promise_type = MyPromise;

public:
    MyCoroutine(std::coroutine_handle<MyPromise> h)
        : h_(h)
    {
        std::cout << "MyCoroutine() " << this << std::endl;
    }

    ~MyCoroutine()
    {
        h_.destroy();
        std::cout << "~MyCoroutine() " << this << std::endl;
    }

    bool isDone() const
    {
        // done() 返回 true，当且仅当 h_ 被挂起，并且是在它的 final_suspend() 被挂起。
        // 若 h_ 执行结束，且在 final_suspend() 没有挂起，该函数也返回 false !!
        // 此时 h_ 的内容（也即 promise）已经析构，调用 h_.destory() 将导致报错！
        return h_.done();
    }

    void resume()
    {
        h_.resume();
    }

private:
    std::coroutine_handle<MyPromise> h_;
};


MyCoroutine inner_func(int n)
{
    std::cout << "inner func " << n << std::endl;
    co_return n;
}

MyCoroutine outer_func()
{
    for(int i = 0; i < 5; ++i)
    {
        std::cout << "await inner_func " << i << std::endl;

        // 总是先创建 inner_func() 的协程对象，并进入 init point ，若协程挂起（在 init point 或执行到中间挂起）或协程结束时，
        // 将回到这里，并执行 outer_func 的协程的 promise_type 的 await_transform() 过程，然后再执行这里的 co_await
        double m = co_await inner_func(i);

        std::cout << "get from innner: " << m << std::endl << std::endl;
    }
}


int main()
{
    MyCoroutine h = outer_func();

    while (not h.isDone())
    {
        std::cout << "do outer_func()" << std::endl;
        h.resume();
        std::cout << "re outer_func()" << std::endl;
    }

    return 0;
}