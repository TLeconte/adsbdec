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

add_executable(adsbdec main.c)
target_link_libraries(adsbdec-lib airspy usb-1.0 m)
target_link_libraries(adsbdec adsbdec-lib)