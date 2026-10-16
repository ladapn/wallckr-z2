# wallckr-z2 vehicle firmware.
#   west build -b nucleo_f401re --shield motherboard_shield \
#     --shield x_nucleo_bnrg2a1 --shield buttonled_shield firmware/main

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wallckr_main LANGUAGES CXX)

add_subdirectory(../lib wallckr_lib)
target_link_libraries(app PRIVATE wallckr_lib)

target_sources(app PRIVATE
    src/main.cpp
    src/ZephyrMotorController.cpp
    src/ZephyrSteeringServo.cpp
    src/ZephyrUltrasonicSensor.cpp
)

# The minimal C++ library has no STL headers; ETL must not reach for them.
target_compile_definitions(app PRIVATE ETL_NO_STL)
