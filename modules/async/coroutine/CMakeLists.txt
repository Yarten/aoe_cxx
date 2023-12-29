aoe_add_library(coroutine
    DEPEND ${3rd-concurrentqueue}
)

aoe_add_executable_test(coroutine)
aoe_add_executable_test(coroutine CASE sized_allocator)
aoe_add_executable_test(coroutine CASE sized_allocator-parallel_delete)
aoe_add_executable_test(coroutine CASE cache)
