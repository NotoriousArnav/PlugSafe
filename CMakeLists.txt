cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(plugsafe C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

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

# Stub TinyUSB (for now - minimal implementation)
# Full TinyUSB requires pio_usb library which isn't available in standard Pico SDK
add_library(tinyusb_stub INTERFACE)
target_include_directories(tinyusb_stub INTERFACE
    ${CMAKE_SOURCE_DIR}
    include
    lib/tinyusb/src
)

# USB Host Module (PlugSafe specific)
add_library(usb_host STATIC
    src/usb_host.c
    src/threat_analyzer.c
    src/hid_monitor.c
)

target_include_directories(usb_host PUBLIC include lib/tinyusb/src ${CMAKE_SOURCE_DIR})
target_link_libraries(usb_host PUBLIC pico_stdlib tinyusb_stub)

# Main Application
add_executable(
  main
  main.c
)

target_include_directories(main PUBLIC include lib/tinyusb/src ${CMAKE_SOURCE_DIR})
target_link_libraries(main pico_stdlib hardware_i2c hardware_pio hardware_irq oled_driver usb_host)

pico_add_extra_outputs(main)
