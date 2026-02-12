# PlugSafe

**Malicious USB device detector for Raspberry Pi Pico.**

PlugSafe is a hardware security tool that detects BadUSB and Rubber Ducky-style keystroke injection attacks. It acts as a USB host, enumerates any device plugged into it, monitors HID behavior in real time, and classifies devices as **SAFE**, **CAUTION**, or **MALICIOUS** — all displayed on a 128x64 OLED screen.

## How It Works

1. Plug a suspicious USB device into PlugSafe
2. PlugSafe enumerates the device and reads its descriptors (VID, PID, class, manufacturer, product, serial)
3. Non-HID devices (flash drives, audio, etc.) are immediately classified as **SAFE**
4. HID keyboards are classified as **CAUTION** and monitored in real time
5. If the keystroke rate exceeds 50 keys/second (impossible for a human), the device is flagged as **MALICIOUS**

A real Rubber Ducky types at 200-800+ keys/second. PlugSafe catches it within the first second.

## Features

- Real-time HID keystroke rate monitoring with 1-second sliding window
- Three-level threat classification: SAFE, CAUTION, MALICIOUS
- 128x64 OLED display showing device info, threat level, and live keystroke rate
- USB hub detection with warning screen
- BOOTSEL button toggles between VID/PID and manufacturer/product views
- Adaptive LED blinking (slow = idle, fast = device connected)
- Full device descriptor parsing (VID, PID, class, manufacturer, product, serial number)
- UART debug output with detailed threat escalation logs
- ~61 KB firmware, bare-metal (no RTOS), runs on a $4 Raspberry Pi Pico

## Hardware

| Component | Specification |
|-----------|--------------|
| MCU | Raspberry Pi Pico (RP2040) |
| Display | 128x64 OLED, I2C (SSD1306 or SH1106) |
| USB | Micro-USB OTG adapter (Pico native port in host mode) |

### Key Pin Assignments

| GPIO | Function |
|------|----------|
| GP20 | OLED SDA (I2C data) |
| GP21 | OLED SCL (I2C clock) |
| GP24 | BOOTSEL button (display mode toggle) |
| GP25 | Built-in LED (status indicator) |

See [docs/HARDWARE.md](docs/HARDWARE.md) for complete wiring and hardware details.

## Quick Start

```bash
# Clone with submodules
git clone --recursive <repo-url>
cd cleanusb

# Build
mkdir build && cd build
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
make -j4

# Flash — hold BOOTSEL on Pico, plug in, then:
cp main.uf2 /media/$USER/RPI-RP2/
```

### Prerequisites

- `arm-none-eabi-gcc` (14.2.0+)
- CMake 3.13+
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)

See [docs/BUILDING.md](docs/BUILDING.md) for detailed build instructions, flashing methods, and debugging setup.

## Documentation

| Document | Description |
|----------|-------------|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | System design, module dependencies, data flow, main loop structure |
| [docs/BUILDING.md](docs/BUILDING.md) | Build prerequisites, commands, flashing, debugging, build system overview |
| [docs/HARDWARE.md](docs/HARDWARE.md) | Bill of materials, pin assignments, wiring diagrams, supported displays |
| [docs/API_REFERENCE.md](docs/API_REFERENCE.md) | Complete API reference for all modules (structs, enums, functions) |
| [docs/THREAT_DETECTION.md](docs/THREAT_DETECTION.md) | Detection pipeline, classification logic, thresholds, design rationale |
| [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | Common issues and solutions for build, display, serial, and USB problems |
| [docs/IMPLEMENTATION_SUMMARY.md](docs/IMPLEMENTATION_SUMMARY.md) | Historical reference for the original GPIO-based USB detection module |

## Project Structure

```
cleanusb/
├── main.c                  Main application (event loop, display, USB polling)
├── tusb_config.h           TinyUSB configuration
├── CMakeLists.txt          Build system
├── include/                Header files for all modules
├── src/                    Source files for all modules
├── lib/tinyusb/            TinyUSB library (git submodule)
└── docs/                   Documentation
```

### Modules

**OLED Display Stack** (`oled_driver` library):
- `oled_i2c` — I2C hardware abstraction
- `oled_driver` — SSD1306/SH1106 controller driver
- `oled_display` — Framebuffer management
- `oled_graphics` — Drawing primitives (pixel, line, rect, circle, bitmap)
- `oled_text` — Text rendering with bitmap fonts
- `oled_font` — 5x7 ASCII bitmap font

**USB Security Stack** (`usb_host` library):
- `usb_host` — TinyUSB host integration, device enumeration, descriptor parsing
- `threat_analyzer` — Threat classification engine
- `hid_monitor` — Keystroke rate detection (1-sec sliding window)

## License

This project is licensed under the **GNU General Public License v3.0** — see the source file headers for details.

Copyright (c) 2026
