# Target-independent code ported from v1's lib/, shared by the apps and the
# tests. Each consumer adds it after find_package(Zephyr):
#   add_subdirectory(<path to firmware/lib> wallckr_lib)
#   target_link_libraries(app PRIVATE wallckr_lib)
# As a Zephyr library it is built with the same compiler flags as the app.

zephyr_library_named(wallckr_lib)

zephyr_library_sources(
    Battery/Battery.cpp
    Communication/ExternalCommandDecoder.cpp
    Motion/Motion.cpp
    Sensing/Sensing.cpp
    Telemetry/RobotPackets.cpp
    Telemetry/RobotPrinter.cpp
)

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
