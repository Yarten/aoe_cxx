//
// Created by yarten on 24-1-3.
//

#pragma once


/**
 * \brief For simplifying the definition of functions and their use in noexcept operator.
 */
#define AOE_NOEXCEPT_BODY(...) \
    noexcept(noexcept(__VA_ARGS__)) { return __VA_ARGS__; }
