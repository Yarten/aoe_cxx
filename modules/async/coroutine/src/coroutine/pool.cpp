//
// Created by yarten on 24-1-7.
//

#include <aoe/async/coroutine/pool.h>
#include "./pool_details/naive_impl.h"


namespace aoe::async::coroutine
{
    class Pool::Impl :
        public pool_details::NaiveImpl
    {
    };

    Pool::Pool() :
        impl_(std::make_unique<Impl>()),
        lifetime_(std::shared_ptr<Pool>(this, [](Pool*){}))
    {

    }

    void Pool::add(Deleter deleter)
    {
    }

    void awake(std::weak_ptr<Pool> pool, std::coroutine_handle<Base> handle)
    {

    }
}
