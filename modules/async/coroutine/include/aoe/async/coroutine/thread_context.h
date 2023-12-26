//
// Created by yarten on 23-12-26.
//

#pragma once

#include <memory>
#include <coroutine>


namespace aoe::async::coroutine
{
    struct ThreadContext
    {

    };

    /**
     * \brief Created by Pool,
     */
    thread_local std::unique_ptr<ThreadContext> thctx_ptr;
}
