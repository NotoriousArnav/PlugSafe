# PlugSafe — Troubleshooting

Common issues and their solutions.

## Build Issues

### "PICO_SDK_PATH not found" or CMake cannot find SDK

Set the environment variable before running CMake:

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
cmake ..
```

Or pass it directly:

```bash
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
```

### ARM toolchain not found

Install the ARM cross-compiler for your platform:

```bash
# Arch Linux
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib

# Ubuntu / Debian
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi

# macOS
brew install arm-none-eabi-gcc
```

Minimum required version: 14.2.0+

### TinyUSB submodule missing or empty

If `lib/tinyusb/` is empty after cloning:

```bash
git submodule update --init --recursive
```

### Build fails with TinyUSB errors

Ensure the TinyUSB submodule is on a compatible commit. The project uses the submodule pinned in `.gitmodules`. If you updated it manually, reset:

```bash
cd lib/tinyusb
git checkout <original-commit>
cd ../..
```

## Display Issues

### OLED shows nothing (blank screen)

1. **Check wiring**: SDA to GP20, SCL to GP21, GND to GND, VCC to 3V3OUT
2. **Check I2C address**: Default is `0x3C`. Some modules use `0x3D`. Change `OLED_ADDRESS` in `main.c`
3. **Check controller type**: Try switching between `OLED_DISPLAY_SSD1306` and `OLED_DISPLAY_SH1106` in the `oled_driver_init()` call in `main.c`
4. **Check power**: Ensure OLED VCC is connected to 3.3V (not 5V, unless the module has a voltage regulator)
5. **Check serial output**: Connect UART and look for `"ERROR: I2C initialization failed"` or `"ERROR: OLED driver initialization failed"`

### Display shows garbled or shifted text

- **Shifted 2 pixels left**: You likely have an SH1106 controller but are using SSD1306 mode. Switch to `OLED_DISPLAY_SH1106`
- **Random pixels / noise**: Check I2C wiring for loose connections. Try adding 4.7k pull-up resistors on SDA and SCL
- **Partial display**: Verify the display is 128x64, not 128x32. The driver assumes 64-pixel height

### LED blinks rapidly at startup then nothing displays

This indicates an initialization error. The `main()` function enters an infinite rapid-blink loop if I2C or driver init fails. Check wiring and I2C address.

The blink patterns for specific failures:
- **100ms on/off**: I2C init failed
- **200ms on/off**: OLED driver init failed
- **300ms on/off**: Display framebuffer init failed (memory allocation)

## Serial / UART Issues

### No serial output visible

1. **Verify connection**: UART TX is on GP0 (default Pico UART0). Connect a USB-to-UART adapter or use a second Pico as a UART bridge
2. **Check baud rate**: The Pico SDK default is 115200 baud. Use:
   ```bash
   picocom /dev/ttyACM0 -b 115200
   # or
   minicom -b 115200 -o -D /dev/ttyACM0
   ```
3. **Check permissions** (Linux):
   ```bash
   sudo usermod -a -G dialout $USER
   # Log out and back in
   ```
4. **Find the device**:
   ```bash
   ls /dev/ttyACM* /dev/ttyUSB*
   ```

### Serial output shows garbled characters

Wrong baud rate. Ensure your terminal is set to 115200, 8N1 (8 data bits, no parity, 1 stop bit).

## USB Detection Issues

### Device plugged in but PlugSafe shows "Waiting..."

1. **OTG adapter required**: The Pico's native micro-USB port must be converted to a host port using a micro-USB OTG adapter or cable
2. **Power supply**: When acting as USB host, the Pico must supply 5V to the connected device. Ensure the Pico is powered from a source that can provide enough current (500+ mA)
3. **Check serial log**: Look for `"USB host initialized"` on startup. If absent, TinyUSB failed to initialize
4. **Device compatibility**: Some USB 3.0-only devices may not enumerate on the RP2040's Full-Speed (12 Mbps) USB host

### "USB HUB DETECTED" warning appears

PlugSafe intentionally warns when a hub is connected. Disconnect the hub and plug the device directly into the Pico's USB port. Hubs are flagged because they could hide malicious devices.

### Device shows as "Unknown Device" with VID:0x0000 PID:0x0000

The device descriptor fetch failed. This can happen with:
- Devices that are very slow to respond during enumeration
- Damaged or non-compliant USB devices
- Insufficient power supply to the device

### Legitimate keyboard shows "CAUTION"

This is expected behavior. All HID keyboards are initially classified as `CAUTION` (potentially unsafe) because PlugSafe cannot distinguish a legitimate keyboard from a Rubber Ducky by descriptors alone. Only after monitoring confirms that the keystroke rate stays below 50 keys/second does the device remain at `CAUTION` (it does not automatically clear to `SAFE`).

A keyboard at `CAUTION` with a low keystroke rate is safe to use.

### Device falsely flagged as "MALICIOUS"

If a legitimate device is flagged, it likely sent a burst of HID reports exceeding 50 per second. This can happen with:
- Devices that send a rapid initialization sequence
- Composite devices that enumerate multiple HID interfaces simultaneously
- Gaming keyboards with macro replay features

The `MALICIOUS` flag is sticky — it does not clear while the device is connected. Disconnect and reconnect the device to reset.

## Known Issues and Caveats

### `usb_detector` module is orphaned

The `usb_detector.c` / `usb_detector.h` files exist in the source tree but are not compiled or used by the current firmware. They are from an earlier GPIO-based detection approach that was replaced by TinyUSB enumeration. See [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) for the original documentation.

### `HID_KEYSTROKE_THRESHOLD_HZ` is defined twice

The constant is defined in both `hid_monitor.h` and `threat_analyzer.h` (both set to 50). If you change the threshold, update both files.

### 8x8 font is not implemented

`oled_get_font_8x8()` is a stub that returns the 5x7 font. Only the 5x7 font is available.

### GP0/GP1 conflict

GP0 and GP1 serve double duty: they are the default UART0 TX/RX pins (used for debug serial output) and also the D+/D- pins in the legacy GPIO detector. Since the GPIO detector is not active, this is not a current issue, but be aware of the conflict if you re-enable it.

### Maximum 4 devices

The firmware tracks at most 4 USB devices simultaneously (`CFG_TUH_DEVICE_MAX = 4`). This is sufficient for the single-port direct-connection use case.
