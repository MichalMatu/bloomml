set(GROWBOX_CLAY_UI_DIR "${CMAKE_CURRENT_LIST_DIR}/../../lib/growbox_clay_ui")
set(GROWBOX_DISPLAY_MODEL_DIR "${CMAKE_CURRENT_LIST_DIR}/../../lib/growbox_display_model")

idf_component_register(
  SRCS
    "${GROWBOX_CLAY_UI_DIR}/src/ClayCore.cpp"
    "${GROWBOX_CLAY_UI_DIR}/src/ClayRenderer.cpp"
    "${GROWBOX_CLAY_UI_DIR}/src/PageRenderer.cpp"
  INCLUDE_DIRS
    "${GROWBOX_CLAY_UI_DIR}/include"
    "${GROWBOX_DISPLAY_MODEL_DIR}/include"
  PRIV_INCLUDE_DIRS
    "${GROWBOX_CLAY_UI_DIR}/src"
)

target_include_directories(
  ${COMPONENT_LIB}
  SYSTEM PRIVATE "${GROWBOX_CLAY_UI_DIR}/third_party/clay/include"
)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic)
