# set the CMake 3.16 or newer
cmake_minimum_required(VERSION 3.16)

# Name the project and select the C language, can also use C++
project(embedded_c_toolkit C)

# Require the C11 language standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build the library using the source file
add_library(embedded_c_toolkit
    src/ring_buffer.c
)

# Tell the compiler where to find the library's header
target_include_directories(embedded_c_toolkit PUBLIC include)


if (MSVC)
    target_compile_options(embedded_c_toolkit PRIVATE /W4 /WX)
else()
    target_compile_options(embedded_c_toolkit PRIVATE -Wall -Wextra -Werror)
endif()

enable_testing()

# Build an executable from the test source.
add_executable(test_ring_buffer tests/test_ring_buffer.c)

# Connext the executable to the library it uses.
target_link_libraries(test_ring_buffer PRIVATE embedded_c_toolkit)

add_test(NAME ring_buffer COMMAND test_ring_buffer)

