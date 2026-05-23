cmake_minimum_required(VERSION 3.5)

project(HackStorm_Clock_App C)

# Include the main TuyaOpen CMakeLists.txt
include(${CMAKE_CURRENT_LIST_DIR}/../../AIoTHackStorm/CMakeLists.txt)

# Add our application source
add_executable(hackstorm_clock
    src/main.c
    src/display_manager.c
    src/http_server.c
    src/time_manager.c
)

# Link libraries
target_link_libraries(hackstorm_clock
    PRIVATE
    tuya_cloud_sdk
    mbedtls
    cjson
)

# Include directories
target_include_directories(hackstorm_clock
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/../../AIoTHackStorm/src
)
