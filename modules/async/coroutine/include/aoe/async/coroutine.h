//
// Created by yarten on 23-12-26.
//

#pragma once

#include "./coroutine/holder.h"


namespace aoe::async
{
    template<class T>
    using Yield = coroutine::Holder<T, void>;

    template<class T = void>
    using Go = coroutine::Holder<void, T>;
}
