aoe_add_library(coroutine)

aoe_add_executable_test(coroutine)
aoe_add_executable_test(coroutine CASE sized_cache)
aoe_add_executable_test(coroutine CASE sized_cache-parallel_delete
    DEPEND ${3rd-concurrentqueue}
)
