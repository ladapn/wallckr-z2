# Target-independent code ported from v1's lib/, shared by the apps and the
# tests. Include it after find_package(Zephyr):
#   include(${CMAKE_CURRENT_LIST_DIR}/../lib/lib.cmake)

set(WALLCKR_LIB_DIR ${CMAKE_CURRENT_LIST_DIR})

target_include_directories(app PRIVATE
    ${WALLCKR_LIB_DIR}/Battery
    ${WALLCKR_LIB_DIR}/Communication
    ${WALLCKR_LIB_DIR}/Control
    ${WALLCKR_LIB_DIR}/IOStream
    ${WALLCKR_LIB_DIR}/Motion
    ${WALLCKR_LIB_DIR}/Robot
    ${WALLCKR_LIB_DIR}/Sensing
    ${WALLCKR_LIB_DIR}/Telemetry
)

target_sources(app PRIVATE
    ${WALLCKR_LIB_DIR}/Battery/Battery.cpp
    ${WALLCKR_LIB_DIR}/Communication/ExternalCommandDecoder.cpp
    ${WALLCKR_LIB_DIR}/Motion/Motion.cpp
    ${WALLCKR_LIB_DIR}/Sensing/Sensing.cpp
    ${WALLCKR_LIB_DIR}/Telemetry/RobotPackets.cpp
    ${WALLCKR_LIB_DIR}/Telemetry/RobotPrinter.cpp
)

# Firmware version sent in StatusPacket, from the app's Zephyr VERSION file
# (0xMMmmppTT: major, minor, patch, tweak). Passed as a define so lib/ needs
# no Zephyr header; apps without a VERSION file (the tests) send 0.
if(DEFINED APPVERSION)
    target_compile_definitions(app PRIVATE WALLCKR_VERSION=${APPVERSION})
else()
    target_compile_definitions(app PRIVATE WALLCKR_VERSION=0)
endif()
