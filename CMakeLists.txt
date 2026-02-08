cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(cleanusb C CXX ASM)
set(CNAME_C_STANDARD 11)
set(CNAME_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

pico_sdk_init()

add_executable(
  main
  main.c
)

target_link_libraries(main pico_stdlib)

pico_add_extra_outputs(main)
