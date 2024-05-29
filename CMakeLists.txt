cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

SET(ProjectName kbcontrol)
SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/michael/projects/pico/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()
project(${ProjectName} C CXX ASM)
# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(${ProjectName}
        kbcontrol.c
        terminal.c
        terminal_commands.c
        usb_descriptors.c
        ${FREERTOS_KERNEL_PATH}/event_groups.c
        split.c
        cobs.c
        crc.c
        )

target_include_directories(${ProjectName} PUBLIC
        include/
        ${CMAKE_CURRENT_LIST_DIR}
        )

target_link_libraries(${ProjectName}
        pico_stdlib
        FreeRTOS-Kernel-Heap4
        tinyusb_device
        tinyusb_board
        )

# Create map/bin/hex file etc
pico_add_extra_outputs(${ProjectName})
