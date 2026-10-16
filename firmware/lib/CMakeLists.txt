# Target-independent code ported from v1's lib/, shared by the apps and the
# tests. Each consumer adds it after find_package(Zephyr):
#   add_subdirectory(<path to firmware/lib> wallckr_lib)
#   target_link_libraries(app PRIVATE wallckr_lib)

# A plain static library: zephyr_library_named() is meant for Zephyr modules
# and warns when an application calls it. Linking zephyr_interface gives it
# the same compiler flags as the app.
add_library(wallckr_lib STATIC
    Battery/Battery.cpp
    Communication/ExternalCommandDecoder.cpp
    Motion/Motion.cpp
    Sensing/Sensing.cpp
    Telemetry/RobotPackets.cpp
    Telemetry/RobotPrinter.cpp
)
target_link_libraries(wallckr_lib PUBLIC zephyr_interface)

# ETL is header-only and target-independent; consumers enable it with
# CONFIG_ETL=y. The minimal C++ library has no STL headers, so ETL must not
# reach for them. Its <new> is only a stub without placement new, so ETL
# must define that itself (ETL_USING_STD_NEW=0) for containers that construct
# elements in place, such as etl::circular_buffer.
target_link_libraries(wallckr_lib PUBLIC etl::etl)
target_compile_definitions(wallckr_lib PUBLIC ETL_NO_STL ETL_USING_STD_NEW=0)

target_include_directories(wallckr_lib PUBLIC
    Battery
    Communication
    Control
    IOStream
    Motion
    Robot
    Sensing
    Telemetry
)

# Firmware version sent in StatusPacket, from the app's Zephyr VERSION file
# (0xMMmmppTT: major, minor, patch, tweak). Passed as a define so lib/ needs
# no Zephyr header; apps without a VERSION file (the tests) send 0. PUBLIC,
# because RobotPackets.h uses it in a header.
if(DEFINED APPVERSION)
    target_compile_definitions(wallckr_lib PUBLIC WALLCKR_VERSION=${APPVERSION})
else()
    target_compile_definitions(wallckr_lib PUBLIC WALLCKR_VERSION=0)
endif()
