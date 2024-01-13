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
        using NaiveImpl::NaiveImpl;
    };

    Pool::Pool() :
        lifetime_(std::shared_ptr<Pool>(this, [](Pool*){})),
        impl_(new Impl(lifetime_))
    {

    }

    Pool::~Pool()
    {
        delete impl_;
    }

    void Pool::add(std::coroutine_handle<Base> handle)
    {
        impl_->addTask(handle);
    }

    void awake(const std::weak_ptr<Pool> _pool, std::coroutine_handle<Base> handle)
    {
        if (handle.address() == nullptr)
            return;

        std::shared_ptr<Pool> pool = _pool.lock();

        if (pool == nullptr)
            return;

        if (handle.promise().setReadyToResume())
            pool->add(handle);
    }
}
