cmake_minimum_required(VERSION 3.30)
project (adsbdec C)

add_compile_options(-O3 -march=native)

add_executable(adsbdec demod.c main.c output.c valid.c )

find_library(LIBAIR airspy)
if(NOT LIBAIR)
message ( FATAL_ERROR "libairspy path not found")
endif()
add_definitions(-DWITH_AIR )
target_sources( adsbdec PRIVATE air.c)
target_link_libraries( adsbdec ${LIBAIR})

target_link_libraries( adsbdec pthread m )

install(TARGETS adsbdec RUNTIME DESTINATION bin)
install(FILES adsbdec.service DESTINATION /lib/systemd/system) 
install(FILES adsbdec.default RENAME adsbdec DESTINATION /etc/default) 

