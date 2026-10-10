cmake_minimum_required(VERSION 3.16)
project(embedded_c_toolkit C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(embedded_c_toolkit
    src/ring_buffer.c
)

target_include_directories(embedded_c_toolkit PUBLIC include)

if (MSVC)
    target_compile_options(embedded_c_toolkit PRIVATE /W4 /WX)
else()
    target_compile_options(embedded_c_toolkit PRIVATE -Wall -Wextra -Werror)
endif()

enable_testing()

add_executable(test_ring_buffer tests/test_ring_buffer.c)
target_link_libraries(test_ring_buffer PRIVATE embedded_c_toolkit)

add_test(NAME ring_buffer COMMAND test_ring_buffer)

