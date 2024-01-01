cmake_minimum_required(VERSION 3.16)
project(_)

# -------------------------------------------------------------
# Import aoe cmake kit
include(FetchContent)
FetchContent_Declare(aoe_cmake_kit GIT_REPOSITORY https://github.com/Yarten/aoe_cmake_kit.git GIT_TAG main)
FetchContent_MakeAvailable(aoe_cmake_kit)

# -------------------------------------------------------------
# Import external packages
aoe_import(https://github.com/cameron314/concurrentqueue v1.0.4
    ALL TO 3rd-concurrentqueue
)
aoe_import(https://github.com/Yarten/aoex_readerwriterqueue master
    ALL TO 3rd-concurrentqueue
)

# -------------------------------------------------------------
# Define the project
aoe_project(
    GIT_ROOT .
    NAME_FROM_GIT
    VERSION_FROM_GIT
    VERSION_NAME_FROM_GIT
)
set(CMAKE_CXX_STANDARD 20)

# Build all modules
aoe_add_subdirectories(modules)

# Complete the project
aoe_project_complete()
