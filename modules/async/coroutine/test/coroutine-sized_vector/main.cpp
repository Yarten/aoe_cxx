//
// Created by yarten on 24-1-4.
//

#include <atomic>
#include <iostream>
#include <optional>
#include <aoe/async/coroutine/pipe_details/sized_vector.h>


int main()
{
    struct A
    {
        std::atomic<int> x;

        A()
        {
            std::cout << "A() " << x.load() << std::endl;
        }

        ~A()
        {
            std::cout << "~A() " << x.load() << std::endl;
        }
    };

    aoe::async::coroutine::pipe_details::SizedVector<A> v(10);

    aoe::async::coroutine::pipe_details::SizedVector<A>::Element element = v[5];
    element.construct();
    element->x.store(2);
    element.destruct();

    v[4].construct();

    struct B
    {
        int & x;
    };

    std::optional<B> x;

    return 0;
}
