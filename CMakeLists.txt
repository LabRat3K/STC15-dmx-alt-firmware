cmake_minimum_required(VERSION 3.5.1)
project(dmx C)

set(CMAKE_C_FLAGS "--std-c99 --opt-code-speed")

include_directories(/usr/share/sdcc/include)

add_executable(dmx main.c dmx.c dip.c leds.c)
