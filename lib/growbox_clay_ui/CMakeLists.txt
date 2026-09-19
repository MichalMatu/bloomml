cmake_minimum_required(VERSION 3.16)
project(growbox_clay_ui LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
  growbox_clay_ui
  STATIC
    src/ClayCore.cpp
    src/ClayRenderer.cpp
    src/HostSimulator.cpp
)

target_include_directories(
  growbox_clay_ui
  PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include"
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/src"
)
target_compile_features(growbox_clay_ui PRIVATE cxx_std_20)
target_compile_options(growbox_clay_ui PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(growbox_clay_sim tools/host_sim_main.cpp)
target_link_libraries(growbox_clay_sim PRIVATE growbox_clay_ui)
target_compile_features(growbox_clay_sim PRIVATE cxx_std_20)
target_compile_options(growbox_clay_sim PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(growbox_clay_ui_tests test/test_main.cpp)
target_link_libraries(growbox_clay_ui_tests PRIVATE growbox_clay_ui)
target_include_directories(
  growbox_clay_ui_tests
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/src"
)
target_compile_features(growbox_clay_ui_tests PRIVATE cxx_std_20)
target_compile_options(growbox_clay_ui_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_library(growbox_clay_public_api_cxx17 OBJECT test/public_api_cxx17.cpp)
target_include_directories(growbox_clay_public_api_cxx17 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
target_compile_features(growbox_clay_public_api_cxx17 PRIVATE cxx_std_17)
target_compile_options(growbox_clay_public_api_cxx17 PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME growbox_clay_ui_tests COMMAND growbox_clay_ui_tests)
add_test(NAME growbox_clay_ui_simulator_smoke COMMAND growbox_clay_sim --self-check)

target_include_directories(growbox_clay_ui SYSTEM PRIVATE "${CMAKE_CURRENT_LIST_DIR}/third_party/clay/include")
target_include_directories(growbox_clay_ui_tests SYSTEM PRIVATE "${CMAKE_CURRENT_LIST_DIR}/third_party/clay/include")
