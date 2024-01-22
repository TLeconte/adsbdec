project(adsbdec)

cmake_minimum_required(VERSION 3.5)

add_library(
        adsbdec-lib
        demod.c
        crc.c
        output.c
        air.c
        valid.c
)

add_subdirectory(airspy)

add_executable(adsbdec main.c)
target_link_libraries(adsbdec-lib usb-1.0 m airspy-static)
target_link_libraries(adsbdec adsbdec-lib)