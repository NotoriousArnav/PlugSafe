# Building PlugSafe

## Prerequisites

- **ARM Cortex-M0+ Toolchain**: `arm-none-eabi-gcc` (14.2.0+)
- **CMake**: 3.13+
- **Pico SDK**: Set `PICO_SDK_PATH` environment variable to your SDK location
- **Python 3**: Required by Pico SDK tools
- **Make**: For the build system
- **Git**: For cloning and submodule management

## Quick Build

```bash
git clone --recursive <repo-url>
cd cleanusb
mkdir build
cd build
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
make -j4
```

If you already cloned without `--recursive`, initialize the TinyUSB submodule:

```bash
git submodule update --init --recursive
```

## Build Output

After successful compilation:

- **main.uf2** (~61 KB) — Firmware ready to flash to Pico
- **main.elf** (~590 KB) — Executable with debug symbols
- **liboled_driver.a** — OLED display driver static library
- **libusb_host.a** — USB host + threat analysis static library

## Flashing to Pico

### Method 1: BOOTSEL Mode (Easiest)

```bash
# 1. Connect Pico to USB while holding BOOTSEL button
# 2. Pico appears as a USB mass storage drive (RPI-RP2)
# 3. Copy the firmware file:
cp build/main.uf2 /media/$USER/RPI-RP2/
# 4. Pico automatically reboots and runs the firmware
```

### Method 2: Using picotool

```bash
# Put Pico in BOOTSEL mode first
picotool load build/main.uf2
# or force-load:
picotool load -f build/main.uf2
```

### Method 3: Using OpenOCD (with debugger)

```bash
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg \
  -c "adapter speed 5000; program {build/main.elf} verify reset exit"
```

## Hardware Setup

### I2C Configuration (OLED Display)

```
Pico GP20      --> OLED SDA (data)
Pico GP21      --> OLED SCL (clock)
Pico GND       --> OLED GND
Pico 3V3OUT    --> OLED VCC
```

See [HARDWARE.md](HARDWARE.md) for complete wiring details including USB connector and button.

### Optional: Pull-up Resistors

If your OLED module lacks onboard pull-up resistors on SDA/SCL:

```
4.7k ohm resistor from SDA (GP20) to 3.3V
4.7k ohm resistor from SCL (GP21) to 3.3V
```

## Supported Displays

- **SSD1306** (128x64) — Most common, used by default
- **SH1106** (128x64) — Automatic +2 column offset handling

To switch controllers, change the init call in `main.c`:

```c
// Default (SSD1306):
oled_driver_init(&driver, OLED_DISPLAY_SSD1306, &i2c);

// Alternative (SH1106):
oled_driver_init(&driver, OLED_DISPLAY_SH1106, &i2c);
```

## Build Configuration Options

Edit `CMakeLists.txt` to customize:

```cmake
# Change optimization level (default: Release)
set(CMAKE_BUILD_TYPE Debug)    # For debugging with symbols

# Change target board
set(PICO_BOARD pico)           # Default
set(PICO_BOARD pico_w)         # For Pico W
```

## Debugging

### Serial Output

The firmware outputs debug messages via UART (GP0/GP1, 115200 baud default).

USB CDC stdio is disabled because the native USB port is used in host mode.

```bash
picocom /dev/ttyACM0 -b 115200
# or
minicom -b 115200 -o -D /dev/ttyACM0
# or
screen /dev/ttyACM0 115200
```

### In-Code Debugging

Use `printf()` — output goes to UART:

```c
printf("Debug: value=%d\n", value);
```

### GDB Debugging

With a CMSIS-DAP or ST-Link debugger:

```bash
arm-none-eabi-gdb build/main.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) break main
(gdb) continue
```

## Build System Overview

### CMakeLists.txt Structure

The project builds two static libraries and one executable:

```cmake
# Library 1: OLED display driver
add_library(oled_driver STATIC
    src/oled_i2c.c
    src/oled_driver.c
    src/oled_display.c
    src/oled_graphics.c
    src/oled_text.c
    src/oled_font.c
)
target_link_libraries(oled_driver pico_stdlib hardware_i2c)

# Library 2: USB host + threat analysis
add_library(usb_host STATIC
    src/usb_host.c
    src/threat_analyzer.c
    src/hid_monitor.c
)
target_link_libraries(usb_host pico_stdlib tinyusb_host tinyusb_board)

# Main executable
add_executable(main main.c)
target_link_libraries(main pico_stdlib hardware_i2c hardware_irq oled_driver usb_host)

# UART enabled, USB CDC disabled (USB port is in host mode)
pico_enable_stdio_uart(main 1)
pico_enable_stdio_usb(main 0)

# Generate UF2, BIN, HEX outputs
pico_add_extra_outputs(main)
```

## Reusing the OLED Driver

The `oled_driver` library has no dependency on USB or threat analysis code. To use it in another Pico project:

1. Copy the OLED files:
   ```bash
   cp src/oled_*.c include/oled_*.h your_project/
   ```

2. Add to your CMakeLists.txt:
   ```cmake
   add_library(oled_driver STATIC
       oled_i2c.c oled_driver.c oled_display.c
       oled_graphics.c oled_text.c oled_font.c
   )
   target_link_libraries(oled_driver pico_stdlib hardware_i2c)

   target_link_libraries(your_app oled_driver)
   ```

3. Include headers:
   ```c
   #include "oled_display.h"
   #include "oled_graphics.h"
   #include "oled_text.h"
   #include "oled_font.h"
   ```

## Troubleshooting

For build and runtime troubleshooting, see [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

### Quick Fixes

**"PICO_SDK_PATH not found"**
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
cmake ..
```

**Compiler not found** — Install ARM toolchain:
```bash
# Arch Linux
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib

# Ubuntu/Debian
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi

# macOS
brew install arm-none-eabi-gcc
```

**TinyUSB submodule empty**
```bash
git submodule update --init --recursive
```

## Performance Notes

- **Build Time**: ~10 seconds (clean build)
- **Binary Size**: ~61 KB (main.uf2)
- **Runtime Memory**: ~37 KB (code + framebuffer)
- **I2C Speed**: 400 kHz (Fast Mode)
- **Display Refresh**: ~10-20 ms (full screen flush)
