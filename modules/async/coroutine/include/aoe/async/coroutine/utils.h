//
// Created by yarten on 23-12-27.
//

#pragma once

#include <memory>


namespace aoe::async::coroutine
{
    /**
     * \brief To check if a weak_ptr<T> is never initialzied (not expried)
     * \see https://stackoverflow.com/questions/45507041/how-to-check-if-weak-ptr-is-empty-non-assigned
     */
    template<class T>
    bool isUnintialized(const std::weak_ptr<T> & ptr)
    {
        using wt = std::weak_ptr<T>;
        return not ptr.owner_before(wt{}) and not wt{}.owner_before(ptr);
    }
}
