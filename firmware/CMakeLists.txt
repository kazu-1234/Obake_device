cmake_minimum_required(VERSION 3.16)

set(PROJECT_VER "0.3.1")
add_compile_definitions(FIRMWARE_VERSION="${PROJECT_VER}")
add_compile_options(-Wno-missing-field-initializers)

# 接続先の上書きは git しない。sdkconfig.defaults.local を置いたときだけ読む。
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sdkconfig.defaults.local")
    set(SDKCONFIG_DEFAULTS
        "${CMAKE_CURRENT_SOURCE_DIR}/sdkconfig.defaults;${CMAKE_CURRENT_SOURCE_DIR}/sdkconfig.defaults.local"
        CACHE STRING "ESP-IDF sdkconfig defaults" FORCE)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(obake-device)
