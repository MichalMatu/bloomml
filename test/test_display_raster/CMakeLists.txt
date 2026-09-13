cmake_minimum_required(VERSION 3.16)
project(growbox_display_raster_tests LANGUAGES CXX)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_executable(
  display_raster_tests
  "${CMAKE_CURRENT_LIST_DIR}/test_main.cpp"
  "${PROJECT_ROOT}/src/climate/display/DisplayMonochromeRaster.cpp"
  "${PROJECT_ROOT}/src/climate/display/Ssd1680FrameMapper.cpp"
)

target_include_directories(display_raster_tests PRIVATE "${PROJECT_ROOT}/src")
target_compile_features(display_raster_tests PRIVATE cxx_std_17)
target_compile_options(display_raster_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME display_raster_tests COMMAND display_raster_tests)
