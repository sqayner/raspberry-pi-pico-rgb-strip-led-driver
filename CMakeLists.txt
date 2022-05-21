cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(RgbLedDriver C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

pico_sdk_init()

add_compile_options(-Wall
        -Wno-format
        -Wno-unused-function
        -Wno-maybe-uninitialized
        )

add_executable(RgbLedDriver
    RgbLedDriver.cpp
)

target_link_libraries(RgbLedDriver
    pico_multicore
    hardware_pwm
    pico_stdlib
)

pico_enable_stdio_usb(RgbLedDriver 1)
pico_enable_stdio_uart(RgbLedDriver 0)

pico_add_extra_outputs(RgbLedDriver)