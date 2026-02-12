# PlugSafe — Hardware Setup

This document covers the bill of materials, wiring, pin assignments, and supported display controllers for PlugSafe.

## Bill of Materials

| Component | Specification | Notes |
|-----------|---------------|-------|
| Raspberry Pi Pico | RP2040-based (Pico or Pico W) | Dual-core ARM Cortex-M0+ @ 133 MHz |
| OLED Display | 128x64 pixels, I2C interface | SSD1306 or SH1106 controller |
| Female USB-A Connector | Standard USB 2.0 Type-A receptacle | For plugging in devices to test |
| Hookup Wire | 22-26 AWG stranded | 4 wires for USB-A, 4 wires for OLED |
| Pull-up Resistors (optional) | 4.7k ohm | Only if OLED module lacks onboard pull-ups |

## Pin Assignments

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| GP0 | USB D+ detection | Input (pull-down) | Green wire from female USB-A. Also default UART TX — shared with debug output |
| GP1 | USB D- detection | Input (pull-down) | White wire from female USB-A. Also default UART RX |
| GP20 | I2C SDA (OLED data) | Bidirectional | `i2c0` instance |
| GP21 | I2C SCL (OLED clock) | Output | `i2c0` instance |
| GP24 | BOOTSEL button | Input (pull-up, active low) | Toggles display mode between VID/PID and Manufacturer views |
| GP25 | Built-in LED | Output | Status blink: slow = idle, fast = device connected |
| Pin 40 | VBUS (5V) | Input (optional) | 5V presence detection from USB connector, not currently used in firmware |

## Wiring Diagram

### OLED Display (I2C)

```
Raspberry Pi Pico                    OLED Display (SSD1306/SH1106)
+-----------------+                  +------------------+
|            GP20 |--- SDA --------->| SDA              |
|            GP21 |--- SCL --------->| SCL              |
|             GND |--- GND --------->| GND              |
|          3V3OUT |--- 3.3V -------->| VCC              |
+-----------------+                  +------------------+
```

I2C address: `0x3C` (default). Some modules use `0x3D` — change `OLED_ADDRESS` in `main.c` if needed.

I2C clock speed: 400 kHz (Fast Mode).

### Female USB-A Connector

```
Raspberry Pi Pico                    Female USB-A Connector
+-----------------+                  +------------------+
|             GP0 |--- Green ------->| D+  (pin 3)      |
|             GP1 |--- White ------->| D-  (pin 2)      |
|             GND |--- Black ------->| GND (pin 4)      |
|            VBUS |--- Red --------->| VBUS (pin 1) 5V  |  (optional)
+-----------------+                  +------------------+
```

The native micro-USB port on the Pico is configured as USB Host (via TinyUSB on port 0). The female USB-A connector wiring above is for the legacy GPIO-based detector module (`usb_detector.c`), which is not currently active. In the current firmware, devices are enumerated through TinyUSB on the Pico's native USB port operated in OTG host mode.

To use the Pico as a USB host through its native port, you need a micro-USB OTG adapter or cable to convert the micro-USB port to a full-size USB-A receptacle.

### Complete Wiring

```
                     Raspberry Pi Pico
                    +------------------+
                    |                  |
   OLED SDA <------| GP20             |
   OLED SCL <------| GP21             |
                    |                  |
   (USB D+) ------>| GP0              |  (legacy GPIO detection)
   (USB D-) ------>| GP1              |  (legacy GPIO detection)
                    |                  |
    BOOTSEL ------>| GP24             |  (button, active low)
                    |                  |
   Built-in LED <--| GP25             |  (status indicator)
                    |                  |
   UART TX ------->| GP0  (shared)    |  (debug serial output)
   UART RX <-------| GP1  (shared)    |  (debug serial input)
                    |                  |
   OLED GND <------| GND              |
   OLED VCC <------| 3V3OUT           |
                    |                  |
   [micro-USB] ----| USB (port 0)     |  TinyUSB host mode (OTG)
                    +------------------+
```

## Supported OLED Controllers

PlugSafe supports two common OLED controllers:

### SSD1306 (Default)

The most common 128x64 OLED controller. Uses horizontal addressing mode. No column offset required.

This is the default in `main.c`:
```c
oled_driver_init(&driver, OLED_DISPLAY_SSD1306, &i2c);
```

### SH1106

An alternative controller found on some OLED modules. Uses page addressing mode (no horizontal addressing command). Applies a +2 column offset automatically to center the 128-pixel display in the SH1106's 132-pixel internal RAM.

To switch to SH1106, change the init call in `main.c`:
```c
oled_driver_init(&driver, OLED_DISPLAY_SH1106, &i2c);
```

### How to Tell Which Controller You Have

- If text appears shifted 2 pixels to the left on SSD1306 mode, try SH1106
- If the display shows garbage or nothing on one mode, try the other
- Check the markings on the OLED module's flex cable or PCB
- SH1106 modules often have a slightly different pin header layout

## Optional Pull-up Resistors

Most breakout OLED modules include onboard 4.7k pull-up resistors on SDA and SCL. If yours does not (bare panels), add external pull-ups:

```
3.3V ---+--- 4.7k ohm ---+--- GP20 (SDA)
        |
        +--- 4.7k ohm ---+--- GP21 (SCL)
```

The Pico's internal pull-ups are enabled by the `oled_i2c_init()` function, but they are weak (~50k ohm). External pull-ups provide more reliable I2C communication, especially at 400 kHz.

## Power Considerations

- The OLED display draws 10-30 mA depending on how many pixels are lit
- The Pico's 3V3OUT pin can supply up to 300 mA, sufficient for the OLED
- When acting as USB host, the Pico must supply 5V VBUS to the connected device — this comes from the Pico's own VBUS pin (micro-USB power input). Ensure the Pico is powered from a source that can supply adequate current (500 mA minimum recommended)
- Total system current draw: ~50-150 mA depending on connected device and display content
