cmake_minimum_required(VERSION 3.16)
project(growbox_display_runtime_tests LANGUAGES CXX)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_executable(
  display_runtime_tests
  "${CMAKE_CURRENT_LIST_DIR}/test_main.cpp"
  "${PROJECT_ROOT}/src/climate/display/DisplayPresenter.cpp"
  "${PROJECT_ROOT}/src/climate/display/DisplayMenu.cpp"
  "${PROJECT_ROOT}/src/climate/runtime/schedule/EuropeWarsawTime.cpp"
)

target_include_directories(
  display_runtime_tests
  PRIVATE
    "${PROJECT_ROOT}/src"
    "${PROJECT_ROOT}/lib/growbox_display_model/include"
    "${PROJECT_ROOT}/lib/environment_control/src"
)

target_compile_features(display_runtime_tests PRIVATE cxx_std_17)
target_compile_options(display_runtime_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME display_runtime_tests COMMAND display_runtime_tests)
