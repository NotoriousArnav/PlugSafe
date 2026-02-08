# Building the OLED Driver Project

## Prerequisites

- **ARM Cortex-M0+ Toolchain**: `arm-none-eabi-gcc` (14.2.0+)
- **CMake**: 3.13+
- **Pico SDK**: Located at `/home/arnav/Code/pico-sdk`
- **Python 3**: For Pico tools
- **Make**: For build system

## Quick Build

```bash
cd /home/arnav/Code/clenausb
rm -rf build
mkdir build
cd build
cmake -DPICO_SDK_PATH=/home/arnav/Code/pico-sdk ..
make -j4
```

## Build Output

After successful compilation, you'll have:

- **main.uf2** (58 KB) - Firmware ready to flash to Pico
- **main.elf** (542 KB) - Executable with debug symbols
- **liboled_driver.a** (1.3 MB) - Reusable driver library

## Flashing to Pico

### Method 1: Using Bootloader (Easiest)

```bash
# 1. Connect Pico to USB while holding BOOTSEL button
# 2. Pico appears as a USB drive
# 3. Copy the firmware file:
cp /home/arnav/Code/clenausb/build/main.uf2 /media/user/RPI-RP2/
# 4. Pico automatically reboots and runs the firmware
```

### Method 2: Using picotool (if installed)

```bash
# Put Pico in BOOTSEL mode
picotool load /home/arnav/Code/clenausb/build/main.uf2
# or
picotool load -f /home/arnav/Code/clenausb/build/main.uf2
```

### Method 3: Using OpenOCD (with debugger)

```bash
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg \
  -c "adapter speed 5000; program {build/main.elf} verify reset exit"
```

## Hardware Setup

### Default I2C Configuration

```
Pico Pin 1 (GP0)  → OLED SDA
Pico Pin 2 (GP1)  → OLED SCL
Pico Pin 3 (GND)  → OLED GND
Pico Pin 36 (3.3V) → OLED VCC
```

### Optional: Pull-up Resistors

If your OLED module doesn't have pull-up resistors on SDA/SCL:

```
4.7kΩ resistor from SDA (Pico GP0) to 3.3V
4.7kΩ resistor from SCL (Pico GP1) to 3.3V
```

## Supported Displays

- **SSD1306** (128×64) - Most common
- **SH1106** (128×64) - With automatic column offset handling

To switch between controllers, edit main.c:

```c
// Line ~65 in main.c
if (!oled_driver_init(&driver, OLED_DISPLAY_SSD1306, &i2c)) {
    // Try SH1106 if SSD1306 fails:
    // oled_driver_init(&driver, OLED_DISPLAY_SH1106, &i2c)
}
```

## Build Configuration Options

Edit `CMakeLists.txt` to customize:

```cmake
# Change optimization level (default: Release)
set(CMAKE_BUILD_TYPE Debug)    # For debugging with symbols

# Change target board
set(PICO_BOARD pico)           # Default
set(PICO_BOARD pico_w)         # For Pico W (WiFi)
```

## Debugging

### Serial Output

The firmware outputs debug messages via UART (GPIO 0/1 at 230400 baud).

To view serial output on Linux:

```bash
minicom -b 230400 -o -D /dev/ttyACM0
# or
picocom /dev/ttyACM0 -b 230400
# or
screen /dev/ttyACM0 230400
```

### In-Code Debugging

Use `printf()` for debug output:

```c
printf("Debug message: %d\n", value);
```

### GDB Debugging

With a CMSIS-DAP or ST-Link debugger connected:

```bash
arm-none-eabi-gdb build/main.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) break main
(gdb) continue
```

## Build System Overview

### CMakeLists.txt Structure

```cmake
# Enable Pico SDK
include(pico_sdk_import.cmake)
pico_sdk_init()

# Create OLED driver static library
add_library(oled_driver STATIC
    src/oled_i2c.c
    src/oled_driver.c
    src/oled_display.c
    src/oled_graphics.c
    src/oled_text.c
    src/oled_font.c
)
target_include_directories(oled_driver PUBLIC include)
target_link_libraries(oled_driver pico_stdlib hardware_i2c)

# Create main executable
add_executable(main main.c)
target_link_libraries(main oled_driver)

# Generate UF2, BIN, HEX outputs
pico_add_extra_outputs(main)
```

## Using OLED Driver in Other Projects

To reuse the OLED driver library in other Pico projects:

1. Copy the OLED driver files to your project:
   ```bash
   cp -r include/ src/ CMakeLists.txt your_project/
   ```

2. In your CMakeLists.txt:
   ```cmake
   add_library(oled_driver STATIC
       src/oled_i2c.c
       src/oled_driver.c
       # ... etc
   )
   target_include_directories(oled_driver PUBLIC include)
   target_link_libraries(oled_driver pico_stdlib hardware_i2c)
   
   add_executable(your_app your_main.c)
   target_link_libraries(your_app oled_driver)
   ```

3. Include headers in your code:
   ```c
   #include "oled_display.h"
   #include "oled_graphics.h"
   #include "oled_text.h"
   ```

## Troubleshooting

### Build Fails: "PICO_SDK_PATH not found"

Set the environment variable:
```bash
export PICO_SDK_PATH=/home/arnav/Code/pico-sdk
cmake ..
```

### Compiler Not Found

Install ARM toolchain:
```bash
# Arch Linux
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib

# Ubuntu/Debian
sudo apt install gcc-arm-none-eabi

# macOS
brew install arm-none-eabi-gcc
```

### UF2 File Won't Copy to Pico

- Ensure Pico is in BOOTSEL mode (hold button while plugging in)
- Check if drive mounted: `mount | grep RPI`
- Try with `-f` flag: `picotool load -f main.uf2`

### Display Shows No Output

1. Check wiring (SDA/SCL/GND/VCC)
2. Verify I2C address (default 0x3C, may be 0x3D)
3. Check if display responds to I2C scan
4. Enable debug output via serial console

### Serial Output Not Visible

1. Install minicom/picocom: `sudo apt install picocom`
2. Check device: `ls -la /dev/ttyACM*`
3. Verify permissions: `sudo usermod -a -G dialout $USER`
4. Restart: `sudo systemctl restart udev`

## Performance Notes

- **Build Time**: ~10 seconds (clean)
- **Binary Size**: 58 KB (main.uf2)
- **Runtime Memory**: ~37 KB (code + framebuffer)
- **I2C Speed**: 400 kHz (standard mode)
- **Display Refresh**: ~10-20 ms (full screen)

## File Locations

- **Source Code**: `/home/arnav/Code/clenausb/`
- **Headers**: `/home/arnav/Code/clenausb/include/`
- **Implementation**: `/home/arnav/Code/clenausb/src/`
- **Example**: `/home/arnav/Code/clenausb/main.c`
- **Build Output**: `/home/arnav/Code/clenausb/build/`

## Next Steps

1. **Flash to Pico**: Follow flashing instructions above
2. **Test Hardware**: Connect display and verify output
3. **Customize**: Edit main.c for your application
4. **Integrate**: Use OLED driver functions in your code

See `OLED_DRIVER_README.md` for API documentation.
