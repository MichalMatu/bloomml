cmake_minimum_required(VERSION 3.16)
project(growbox_display_async_tests LANGUAGES CXX)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_executable(display_async_tests "${CMAKE_CURRENT_LIST_DIR}/test_main.cpp")
target_include_directories(display_async_tests PRIVATE "${PROJECT_ROOT}/src")
target_compile_features(display_async_tests PRIVATE cxx_std_17)
target_compile_options(display_async_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME display_async_tests COMMAND display_async_tests)
