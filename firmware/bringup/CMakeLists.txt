# wallckr-z2 hardware bring-up firmware.
#   west build -b nucleo_f401re --shield motherboard_shield \
#     --shield x_nucleo_bnrg2a1 --shield buttonled_shield firmware/bringup

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wallckr_bringup LANGUAGES CXX)

target_sources(app PRIVATE
    src/main.cpp
    src/analog.cpp
    src/ble.cpp
    src/buttons.cpp
    src/encoder.cpp
    src/leds.cpp
    src/motor.cpp
    src/servo.cpp
    src/ultrasonic.cpp
)

# The minimal C++ library has no STL headers; ETL must not reach for them.
target_compile_definitions(app PRIVATE ETL_NO_STL)
