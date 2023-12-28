//
// Created by yarten on 23-12-28.
//

#include <iostream>
#include <thread>
#include <concurrentqueue.h>
#include <list>
#include <aoe/async/coroutine/sized_cache.h>


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
    moodycamel::ConcurrentQueue<MyClass *> ptrs;
    std::counting_semaphore<> sem(0);

    aoe::async::coroutine::SizedCache<MyClass> cache(10, 5);

    std::atomic_bool is_last_one_missing = true;
    std::atomic_int dequeue_failure_count = 0;

    std::list<std::thread> rece_ths;
    for(int i = 0; i < 8; ++i)
    {
        rece_ths.emplace_back(
            [&, i]()
            {
                std::cout << "delete thread " << i << " begin ..." << std::endl;

                int count = 0;

                while (true)
                {
                    if (not sem.try_acquire_for(std::chrono::seconds(3)))
                        break;

                    MyClass * ptr = nullptr;
                    if (ptrs.try_dequeue(ptr))
                    {
                        if (ptr->n_ == 99999)
                            is_last_one_missing = false;

                        cache.release(ptr);
                        ++count;
                    }
                    else
                        ++dequeue_failure_count;
                }

                std::cout << "delete thread " << i << " end." << std::endl;
            }
        );
    }

    std::atomic_bool is_last_one_sent = false;

    std::thread create_th(
        [&]()
        {
            std::cout << "create thread begin ..." << std::endl;

            for(int i = 1; i < 100000; ++i)
            {
                MyClass * ptr = cache.tryNew(
                    [i](void * mem_ptr)
                    {
                        new(mem_ptr) MyClass(i);
                    }
                );

                if (ptr == nullptr)
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                else
                {
                    if (ptrs.enqueue(ptr))
                    {
                        if (ptr->n_ == 99999)
                            is_last_one_sent = true;

                        sem.release();
                    }
                }
            }

            std::cout << "create thread end." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    );

    create_th.join();

    for(std::thread & i : rece_ths)
        i.join();

    std::cout << std::flush;
    std::cout << "clear cache, should be empty ..." << std::endl;

    std::cout << "last one sent: " << is_last_one_sent << std::endl;
    std::cout << "last one missing: " << is_last_one_missing << std::endl;

    MyClass * last_one_ptr = nullptr;
    for(int i = 0; i < 10; ++i)
    {
        if (ptrs.try_dequeue(last_one_ptr))
        {
            std::cout << "last one in queue !" << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "dequeue failure count: " << dequeue_failure_count.load() << std::endl;

    cache.fastClear();

    return 0;
}
