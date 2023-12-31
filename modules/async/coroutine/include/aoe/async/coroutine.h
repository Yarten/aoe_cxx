//
// Created by yarten on 23-12-26.
//

#pragma once

#include "./coroutine/holder.h"
#include "./coroutine/pipe.h"
#include "./coroutine/selector.h"
#include "./coroutine/pool.h"


namespace aoe::async
{
    template<class T>
    using Yield = coroutine::Holder<T, void>;

    template<class T = void>
    using Go = coroutine::Holder<void, T>;

    template<class ... TAwaiter>
    using select = coroutine::Selector<TAwaiter...>;

    template<class T>
    using Pipe = coroutine::Pipe<T>;

    inline coroutine::Pool go;
}
