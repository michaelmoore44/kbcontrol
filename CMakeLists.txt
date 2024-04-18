cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/michael/projects/pico/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()
project(kbcontrol C CXX ASM)
# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(kbcontrol kbcontrol.c)

pico_enable_stdio_uart(kbcontrol 1)
pico_enable_stdio_usb(kbcontrol 0)

target_link_libraries(kbcontrol pico_stdlib)

# Create map/bin/hex file etc
pico_add_extra_outputs(kbcontrol)
