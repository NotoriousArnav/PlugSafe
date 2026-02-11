cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(plugsafe C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Point Pico SDK to our local TinyUSB (MUST be before pico_sdk_init)
set(PICO_TINYUSB_PATH ${CMAKE_SOURCE_DIR}/lib/tinyusb)

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

# USB Host Module (PlugSafe specific)
add_library(usb_host STATIC
    src/usb_host.c
    src/threat_analyzer.c
    src/hid_monitor.c
)

target_include_directories(usb_host PUBLIC
    include
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(usb_host PUBLIC
    pico_stdlib
    tinyusb_host
    tinyusb_board
)

# Main Application
add_executable(
  main
  main.c
)

target_include_directories(main PUBLIC include ${CMAKE_SOURCE_DIR})
target_link_libraries(main pico_stdlib hardware_i2c hardware_irq oled_driver usb_host)

# Use UART for stdio (native USB is in host mode)
pico_enable_stdio_uart(main 1)
pico_enable_stdio_usb(main 0)

pico_add_extra_outputs(main)
