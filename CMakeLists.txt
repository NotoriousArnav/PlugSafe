cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(plugsafe C CXX ASM)
set(CNAME_C_STANDARD 11)
set(CNAME_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

pico_sdk_init()

# OLED Driver Library (reusable module)
add_library(oled_driver STATIC
    src/oled_i2c.c
    src/oled_driver.c
    src/oled_display.c
    src/oled_graphics.c
    src/oled_text.c
    src/oled_font.c
)

target_include_directories(oled_driver PUBLIC include)
target_link_libraries(oled_driver PUBLIC pico_stdlib hardware_i2c)

# Main Application
add_executable(
  main
  main.c
)

target_link_libraries(main pico_stdlib hardware_i2c oled_driver)

pico_add_extra_outputs(main)
