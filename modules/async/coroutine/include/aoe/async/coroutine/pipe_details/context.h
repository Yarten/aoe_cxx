//
// Created by yarten on 23-12-31.
//

#pragma once

#include <exception>


namespace aoe::async::coroutine::pipe_details
{
    inline constexpr unsigned INVALID_ID = -1;

    struct SendId
    {
        unsigned num = INVALID_ID;
    };

    struct RecvId
    {
        unsigned num = INVALID_ID;
    };

    template<class T>
    class Context
    {
    public:
        virtual ~Context() = default;

        virtual SendId nextSendId() noexcept = 0;

        virtual RecvId nextRecvId() noexcept = 0;


    };
}
