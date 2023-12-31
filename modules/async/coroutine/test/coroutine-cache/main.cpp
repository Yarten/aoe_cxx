//
// Created by yarten on 23-12-29.
//

#include <iostream>
#include <thread>
#include <list>
#include <aoe/async/coroutine/cache.h>


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
    using namespace aoe::async::coroutine;

    GlobalCache<MyClass> global_cache;

    // one thread cache test
    const Cache<MyClass>::Config config
    {
        .bin_block_size = 2,
        .bin_depth = 2
    };
    Cache<MyClass> cache = global_cache.makeCache(config);

    const std::size_t SIZE      = (1 << (config.bin_depth - 1)) * config.bin_block_size;
    const std::size_t HALF_SIZE = SIZE >> 1;

    std::list<CacheElement<MyClass>> elements;
    int n = 0;

    auto push = [&]()
    {
        elements.push_back(cache.next());
        elements.back().construct(++n);
    };

    auto pop = [&]()
    {
        std::move(elements.front()).destruct();
        elements.pop_front();
    };

    std::cout << "1. cache in and cache out" << std::endl;
    for(std::size_t i = 0; i < SIZE; ++i)
        push();

    for(std::size_t i = 0; i < SIZE; ++i)
        pop();

    std::cout << "2. cache in many to expand" << std::endl;
    for(std::size_t i = 0; i < SIZE + HALF_SIZE; ++i)
        push();

    std::cout << "3. cache out many to clean up" << std::endl;
    for(std::size_t i = 0; i < HALF_SIZE; ++i)
        pop();

    std::cout << "4. cache in to reuse the cleaned up bin" << std::endl;
    for(std::size_t i = 0; i < SIZE; ++i)
        push();

    std::cout << "5. cache in one full bin and cache out two bin, now the cleaning queue has two bins." << std::endl;

    for(std::size_t i = 0; i < SIZE; ++i)
        push();

    for(std::size_t i = 0; i < SIZE * 2; ++i)
        pop();

    std::cout << "6. cache in, this time, a bin will be commited to the spare queue." << std::endl;
    for(std::size_t i = 0; i < SIZE; ++i)
        push();

    std::cout << "7. cache in again, this time, the bin in the spare queue will be used." << std::endl;
    for(std::size_t i = 0; i < SIZE; ++i)
        push();

    std::cout << "END" << std::endl;

    return 0;
}
