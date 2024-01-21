aoe_add_library(coroutine
    DEPEND trait macro panic ${3rd-concurrentqueue}
)

add_compile_options(-fconcepts-diagnostics-depth=2)

aoe_add_test(coroutine)
aoe_add_test(coroutine CASE sized_allocator)
aoe_add_test(coroutine CASE sized_allocator-parallel_delete)
aoe_add_test(coroutine CASE cache)
aoe_add_test(coroutine CASE pipe)
aoe_add_test(coroutine CASE selector)
aoe_add_test(coroutine CASE sized_vector)
aoe_add_test(coroutine CASE pool)
aoe_add_test(coroutine CASE one_to_one_pipe)
aoe_add_test(coroutine CASE one_to_one_pipe-faster_sender)
aoe_add_test(coroutine CASE one_to_one_pipe-faster_receiver)
aoe_add_test(coroutine CASE one_to_one_pipe-select_senders)
aoe_add_test(coroutine CASE one_to_one_pipe-select_receivers)
aoe_add_test(coroutine CASE one_to_one_pipe-zero_buffer)
