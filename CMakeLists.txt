cmake_minimum_required(VERSION 3.13)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

project(rfid_rw_7941w C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

# Project files

add_executable(main
src/main.c
src/ImageData.c

src/lib/7941w.c

src/lib/waveshare/Config/DEV_config.c
src/lib/waveshare/Fonts/font12.c
src/lib/waveshare/Fonts/font12CN.c
src/lib/waveshare/Fonts/font16.c
src/lib/waveshare/Fonts/font20.c
src/lib/waveshare/Fonts/font24.c
src/lib/waveshare/Fonts/font24CN.c
src/lib/waveshare/Fonts/font8.c
src/lib/waveshare/GUI/GUI_Paint.c
src/lib/waveshare/LCD/LCD_1in3.c

)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/lib/waveshare/Config
    ${CMAKE_CURRENT_LIST_DIR}/src/lib/waveshare/Fonts
    ${CMAKE_CURRENT_LIST_DIR}/src/lib/waveshare/GUI
    ${CMAKE_CURRENT_LIST_DIR}/src/lib/waveshare/LCD
)
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)

pico_add_extra_outputs(main)

target_link_libraries(main pico_stdlib hardware_spi hardware_i2c hardware_pwm hardware_adc)
