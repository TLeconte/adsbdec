project(adsbdec)

cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(FindPackageHandleStandardArgs)

add_library(
        adsbdec-lib
        demod.c
        crc.c
        output.c
        air.c
        valid.c
)

add_subdirectory(airspy)

find_package(LIBUSB REQUIRED)

include_directories(${LIBUSB_INCLUDE_DIR})
include_directories(airspy/libairspy/src)

add_executable(adsbdec main.c)
target_link_libraries(adsbdec-lib m airspy-static ${LIBUSB_LIBRARIES})
target_link_libraries(adsbdec adsbdec-lib)