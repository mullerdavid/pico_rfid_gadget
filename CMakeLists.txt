cmake_minimum_required(VERSION 3.13)

# Pull in PICO SDK (must be before project)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

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
project(rfid_gadget C CXX ASM)

# 7941w library
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/pico_7941w/lib/7941w)

# Waveshare library
set(WAVESHARE_LIBS Config Fonts GUI LCD)
foreach(LIB_ITEM Config Fonts GUI LCD)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/Waveshare_Pico_code/c/lib/${LIB_ITEM})
    target_include_directories(
        ${LIB_ITEM} INTERFACE 
        include ${CMAKE_CURRENT_LIST_DIR}/lib/Waveshare_Pico_code/c/lib/${LIB_ITEM}
    )
    target_compile_options(
        ${LIB_ITEM} PRIVATE 
        -Wno-unused-variable
        -Wno-parentheses
    )
endforeach()

add_executable(main
    src/ImageData.c
    src/main.c
)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)

pico_add_extra_outputs(main)

target_link_libraries(main pico_multicore 7941w ${WAVESHARE_LIBS}) 
