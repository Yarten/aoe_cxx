//
// Created by yarten on 24-1-9.
//

#include <aoe/macro.h>


int main()
{
    {
        constexpr int N = 999;

        static_assert(AOE_COUNT_ARGS() == 0);
        static_assert(AOE_COUNT_ARGS(1) == 1);
        static_assert(AOE_COUNT_ARGS(1, 2) == 2);
        static_assert(AOE_COUNT_ARGS(1, 2, 3) == 3);

        static_assert(AOE_COUNT_ARGS_0_N() == 0);
        static_assert(AOE_COUNT_ARGS_0_N(1) == N);
        static_assert(AOE_COUNT_ARGS_0_N(1, 2) == N);
        static_assert(AOE_COUNT_ARGS_0_N(1, 2, 3) == N);

        static_assert(AOE_COUNT_ARGS_0_1_N() == 0);
        static_assert(AOE_COUNT_ARGS_0_1_N(1) == 1);
        static_assert(AOE_COUNT_ARGS_0_1_N(1, 2) == N);
        static_assert(AOE_COUNT_ARGS_0_1_N(1, 2, 3) == N);
    }

    {
        static_assert(AOE_CONCAT(1, 2) == 12);
        static_assert(AOE_CONCAT(1, 2, 3) == 123);
    }

    {
#define MY_MACRO(x) +x

        static_assert(AOE_INVOKE(MY_MACRO, 1, 2, 3) == 6);

#define MY_MACRO_2(x, y) +(x * y)

        // fixed one parameter '4', expand the macro with '1', '2', '3'
        static_assert(AOE_INVOKE_WITH(MY_MACRO_2, 1, 4, 1, 2, 3) == 24);

#define MY_MACRO_3(x, y, z) +(x * y * z)

        // fixed first two parameters '4' and '5', expand the macro with the remains
        static_assert(AOE_INVOKE_WITH(MY_MACRO_3, 2, 4, 5, 1, 2, 3) == 120);
    }

    return 0;
}
