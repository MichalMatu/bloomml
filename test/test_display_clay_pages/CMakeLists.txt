cmake_minimum_required(VERSION 3.16)
project(growbox_display_clay_pages_tests LANGUAGES CXX)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_executable(
  display_clay_pages_tests
  "${CMAKE_CURRENT_LIST_DIR}/test_main.cpp"
  "${PROJECT_ROOT}/src/climate/display/DisplayPresenter.cpp"
  "${PROJECT_ROOT}/src/climate/runtime/schedule/EuropeWarsawTime.cpp"
  "${PROJECT_ROOT}/lib/growbox_clay_ui/src/ClayCore.cpp"
  "${PROJECT_ROOT}/lib/growbox_clay_ui/src/ClayRenderer.cpp"
  "${PROJECT_ROOT}/lib/growbox_clay_ui/src/HostSimulator.cpp"
)

target_include_directories(
  display_clay_pages_tests
  PRIVATE
    "${PROJECT_ROOT}/src"
    "${PROJECT_ROOT}/lib/environment_control/src"
    "${PROJECT_ROOT}/lib/growbox_display_model/include"
    "${PROJECT_ROOT}/lib/growbox_clay_ui/include"
    "${PROJECT_ROOT}/lib/growbox_clay_ui/src"
)
target_include_directories(
  display_clay_pages_tests
  SYSTEM PRIVATE "${PROJECT_ROOT}/lib/growbox_clay_ui/third_party/clay/include"
)
target_compile_features(display_clay_pages_tests PRIVATE cxx_std_20)
target_compile_options(display_clay_pages_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
if(UNIX)
  target_link_libraries(display_clay_pages_tests PRIVATE m)
endif()

enable_testing()
add_test(NAME display_clay_pages_tests COMMAND display_clay_pages_tests)
