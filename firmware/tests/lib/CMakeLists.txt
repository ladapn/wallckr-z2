# Unit tests for firmware/lib (the code ported from v1's lib/), run on native_sim:
#   west twister -p native_sim -T firmware/tests

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wallckr_lib_tests LANGUAGES CXX)

add_subdirectory(../../lib wallckr_lib)
target_link_libraries(app PRIVATE wallckr_lib)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../common)

target_sources(app PRIVATE
    src/test_auto_steering.cpp
    src/test_battery.cpp
    src/test_command_decoder.cpp
    src/test_filter.cpp
    src/test_regulator.cpp
    src/test_robot.cpp
    src/test_robot_packets.cpp
)
