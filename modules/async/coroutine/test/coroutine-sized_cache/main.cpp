//
// Created by yarten on 23-12-28.
//

#include <iostream>
#include <memory>
#include <algorithm>
#include <random>
#include <aoe/async/coroutine/sized_cache.h>

#include <vector>


struct MyClass
{
    explicit MyClass(int n)
        : n_(n)
    {
        std::cout << "MyClass() " << n_ << std::endl;
    }

    ~MyClass()
    {
        std::cout << "~MyClass() " << n_ << std::endl;
    }

    int n_ = -1;
};


int main()
{
    aoe::async::coroutine::SizedCache<MyClass> cache(5, 4);

    int m = 1;

    std::cout << "begin to cache ..." << std::endl;

    std::vector<MyClass *> ptrs;

    while (true)
    {
        MyClass * ptr = cache.tryNew(
            [&](void * mem_ptr)
            {
                new(mem_ptr) MyClass(m++);
            }
        );

        if (ptr == nullptr)
            break;

        ptrs.push_back(ptr);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::ranges::shuffle(ptrs, g);

    std::cout << "destory cache in random order ..." << std::endl;
    for(std::size_t i = 0, size = ptrs.size(); i + 5 < size; ++i)
        cache.release(ptrs[i]);

    std::cout << "clear cache ..." << std::endl;
    cache.fastClear();

    return 0;
}
