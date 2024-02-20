cmake_minimum_required (VERSION 3.2)
project (adsbdec C)

add_compile_options(-O3 -march=native)

add_executable(adsbdec demod.c main.c crc.c output.c valid.c )

option(rtl "Compiling for rtl sdr" )
if(rtl)
find_library(LIBRTL rtlsdr)
if(NOT LIBRTL)
message (FATAL_ERROR "librtlsdr path not found")
endif()
add_definitions(-DWITH_RTL )
target_sources( adsbdec PRIVATE rtl.c)
target_link_libraries( adsbdec ${LIBRTL})
endif()

option(airspy "Compiling for airspy sdr" )
if(airspy)
find_library(LIBAIR airspy)
if(NOT LIBAIR)
message ( FATAL_ERROR "libairspy path not found")
endif()
add_definitions(-DWITH_AIR )
target_sources( adsbdec PRIVATE air.c)
target_link_libraries( adsbdec ${LIBAIR})
endif()

if(rtl AND airspy )
message ("Sorry, could not compile for rtl AND airspy")
message ("try : cmake -Drtl=ON -Dairspy=OFF" ..)
message ("or  cmake -Drtl=OFF -Dairspy=ON" ..)
endif()

if(NOT rtl AND NOT airspy )
message ("No sdr option set ! Are you sure ?")
message ("try cmake -Drtl=ON or -Dairspy=ON ..")
endif()

target_link_libraries( adsbdec pthread m )

install(TARGETS adsbdec RUNTIME DESTINATION bin)
