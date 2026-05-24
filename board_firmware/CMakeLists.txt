# @file CMakeLists.txt
# @brief HackStorm Smart Alarm Clock Application

set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})

# Collect source files
aux_source_directory(${APP_PATH}/src APP_SRCS)

# Configure target
add_library(tuyaapp STATIC
    ${APP_SRCS}
)

# Set include directories
target_include_directories(tuyaapp
    PRIVATE
    ${APP_PATH}/src
    ${CMAKE_SOURCE_DIR}/src/peripherals/display/tdl_display/include
    ${CMAKE_SOURCE_DIR}/src/peripherals/audio_codecs/tdl_audio/include
    ${CMAKE_SOURCE_DIR}/src/libcjson/cJSON
)

# Link dependencies
target_link_libraries(tuyaapp
    tal_system
    tal_wifi
    board_init
    tdl_display
    audio_codecs
    tuyaos_adapter
)
