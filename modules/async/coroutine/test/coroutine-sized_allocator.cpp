//
// Created by yarten on 23-12-28.
//

#include <iostream>
#include <memory>
#include <algorithm>
#include <random>
#include <aoe/async/coroutine/cache_details/sized_allocator.h>

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
    aoe::async::coroutine::cache_details::SizedAllocator<MyClass> alloc(5, 4);

    int m = 1;

    std::cout << "begin to alloc ..." << std::endl;

    std::vector<MyClass *> ptrs;

    while (true)
    {
        void * mem_ptr = alloc.tryAllocate();

        if (mem_ptr == nullptr)
            break;

        ptrs.push_back(new(mem_ptr) MyClass(m++));
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::ranges::shuffle(ptrs, g);

    std::cout << "destory alloc in random order ..." << std::endl;
    for(std::size_t i = 0, size = ptrs.size(); i + 5 < size; ++i)
    {
        ptrs[i]->~MyClass();
        alloc.deallocate(ptrs[i]);
        ptrs[i] = nullptr;
    }

    std::cout << "clear alloc ..." << std::endl;
    alloc.fastClear();

    return 0;
}
