# PlugSafe — Architecture

This document describes the system architecture of PlugSafe: how the firmware is structured, how modules interact, and how data flows from USB enumeration through threat analysis to the OLED display.

## System Overview

PlugSafe is a bare-metal (no RTOS) firmware running on the RP2040 microcontroller. It operates as a USB host that enumerates any device plugged into it, monitors HID behavior in real time, and classifies devices into three threat levels. Results are displayed on a 128x64 OLED screen.

```
+---------------------+
|   USB Device        |  Plugged into female USB-A connector
+---------------------+
         |
         | USB Full-Speed (D+/D-)
         v
+---------------------+
|   RP2040 (Pico)     |
|                     |
|  +----- USB Host ---+---> TinyUSB Stack (port 0, native micro-USB via OTG)
|  |                  |
|  |  +-- Threat -----+---> Classification engine (SAFE / CAUTION / MALICIOUS)
|  |  |  Analyzer     |
|  |  |               |
|  |  +-- HID --------+---> Keystroke rate monitor (1-sec sliding window)
|  |     Monitor      |
|  |                  |
|  +----- OLED -------+---> 128x64 SSD1306/SH1106 via I2C (GP20/GP21)
|        Display      |
|                     |
|  +----- LED --------+---> GP25 (status blink)
|  +----- BOOTSEL ----+---> GP24 (display mode toggle)
|  +----- UART -------+---> Debug output (GP0/GP1 default UART)
+---------------------+
```

## Static Libraries

The build produces two static libraries plus the main executable.

### Library 1: `oled_driver`

A reusable, layered OLED display driver stack. Has no dependency on USB or security logic.

```
oled_i2c          (I2C hardware abstraction)
    |
oled_driver       (SSD1306/SH1106 controller commands)
    |
oled_display      (framebuffer management, flush)
    |
    +--- oled_graphics  (pixel, line, rect, circle, bitmap)
    |
    +--- oled_text      (character/string rendering)
              |
         oled_font      (5x7 bitmap font data)
```

**Source files:** `oled_i2c.c`, `oled_driver.c`, `oled_display.c`, `oled_graphics.c`, `oled_text.c`, `oled_font.c`
**Links against:** `pico_stdlib`, `hardware_i2c`

### Library 2: `usb_host`

The USB security analysis stack. Handles device enumeration, HID monitoring, and threat classification.

```
usb_host          (TinyUSB integration, device enumeration, descriptor parsing)
    |
    +--- threat_analyzer  (threat classification engine)
    |         |
    +--- hid_monitor      (keystroke rate tracking)
```

**Source files:** `usb_host.c`, `threat_analyzer.c`, `hid_monitor.c`
**Links against:** `pico_stdlib`, `tinyusb_host`, `tinyusb_board`

### Main Executable

**Source:** `main.c`
**Links against:** `pico_stdlib`, `hardware_i2c`, `hardware_irq`, `oled_driver`, `usb_host`

## Module Dependency Graph

```
main.c
  |
  +--- oled_i2c.h
  +--- oled_driver.h ----> oled_i2c.h, oled_config.h
  +--- oled_display.h ---> oled_driver.h, oled_config.h
  +--- oled_graphics.h --> oled_display.h, oled_config.h
  +--- oled_text.h ------> oled_display.h, oled_graphics.h
  +--- oled_font.h ------> oled_text.h
  |
  +--- usb_host.h
  +--- threat_analyzer.h -> usb_host.h (for usb_device_info_t)
  +--- hid_monitor.h       (standalone, Pico SDK time only)
```

Note: `usb_host.c` includes `threat_analyzer.h` and `hid_monitor.h` for callbacks, but the header `usb_host.h` does not include them. This avoids circular header dependencies.

## Runtime Data Flow

### TinyUSB Callback Chain

When a USB device is plugged in, TinyUSB fires a sequence of callbacks implemented in `usb_host.c`:

```
USB Device Plugged In
        |
        v
tuh_mount_cb(dev_addr)                          [usb_host.c]
  |-- Fetch device descriptor (VID, PID, class)
  |-- Fetch string descriptors (manufacturer, product, serial)
  |-- Parse UTF-16LE strings to UTF-8
  +-- threat_add_device(dev_info)                [threat_analyzer.c]
        +-- Initial classification:
             Non-HID        --> THREAT_SAFE
             HID Mouse      --> THREAT_SAFE
             HID Keyboard   --> THREAT_POTENTIALLY_UNSAFE
             HID Unknown    --> THREAT_POTENTIALLY_UNSAFE
        |
        v
tuh_hid_mount_cb(dev_addr, instance, ...)        [usb_host.c]
  |-- Record HID protocol (keyboard=1, mouse=2, other=0)
  |-- hid_monitor_add_device(dev_addr)           [hid_monitor.c]  (keyboards only)
  |-- threat_update_device_info(dev_info)        [threat_analyzer.c]
  +-- tuh_hid_receive_report() — start listening
        |
        v
tuh_hid_report_received_cb(dev_addr, ...)        [usb_host.c]  (continuous, every report)
  |-- hid_monitor_report(dev_addr, report, len)  [hid_monitor.c]
  |     +-- Increment report counter
  |     +-- Every 1000ms: calculate current_rate_hz
  |     +-- Update peak_rate_hz
  |
  |-- threat_update_hid_activity(dev_addr, len)  [threat_analyzer.c]
  |     +-- Read rate from hid_get_keystroke_rate()
  |     +-- If rate > 50 Hz: escalate to THREAT_MALICIOUS (sticky)
  |
  +-- tuh_hid_receive_report() — request next report
        |
        v
tuh_umount_cb(dev_addr)                          [usb_host.c]  (on disconnect)
  |-- threat_remove_device(dev_addr)
  |-- hid_monitor_remove_device(dev_addr)
  +-- Clear device slot
```

### Main Event Loop

The `main()` function runs a single-threaded event loop at ~1ms granularity:

```
while (1) {
    now = time_us_64() / 1000

    [Every 200ms]  BOOTSEL button check
                   |-- Debounce, detect rising edge
                   +-- Toggle display mode (VID/PID <-> Manufacturer/Product)

    [Every 10ms]   USB Host polling
                   +-- usb_host_task() -> tuh_task()
                       (processes TinyUSB events, fires callbacks above)

    [Continuous]   Device count edge detection
                   +-- If count changed: force immediate display refresh

    [Every 200ms]  Display refresh
                   |-- If hub connected:     draw_hub_warning_page()
                   |-- Else if device > 0:   draw_device_screen()
                   |-- Else:                 draw_welcome_screen()
                   +-- oled_display_flush() -> I2C write 1024 bytes

    [Every 200ms]  LED update
                   |-- Device connected: fast blink (200ms period)
                   +-- No device: slow blink (500ms period)

    sleep_ms(1)
}
```

## Display State Machine

```
                    +-----------------+
         No device  |    WELCOME      |  "Insert USB Device"
         +--------->|    SCREEN       |  "Waiting..."
         |          +-----------------+
         |                  |
         |          Device plugged in
         |                  |
         |                  v
         |          +-----------------+
         |          |  DEVICE INFO    |  VID/PID or Manufacturer mode
         +----------| (mode toggle    |  Threat level + keystroke rate
  Device removed    |  via BOOTSEL)   |
                    +-----------------+
                            |
                    Hub detected (any time)
                            |
                            v
                    +-----------------+
                    |  HUB WARNING    |  "USB HUB DETECTED"
                    |  PAGE           |  "Please disconnect hub"
                    +-----------------+
```

### Device Info Screen — Two Modes

**Mode 1 (VID/PID)** — Default:
```
Device Detected!
Logitech Keyboard
VID:0x046D PID:0xC31C
Class: 0x03 KBD
Threat: CAUTION
Rate:2 k/s          BOOTSEL
```

**Mode 2 (Manufacturer)** — Press BOOTSEL to toggle:
```
Device Detected!
Logitech Inc.
USB Keyboard
4A3B2C1D
Threat: CAUTION
Rate:2 k/s          BOOTSEL
```

## Design Decisions

### Why MALICIOUS is sticky (never de-escalates)

Once a device is classified as `THREAT_MALICIOUS`, it stays that way until disconnected. A real Rubber Ducky attack takes only a fraction of a second to inject a payload. If the device slowed down after the burst, de-escalating would hide the fact that an attack already occurred.

### Why mice are excluded from keystroke monitoring

Normal mouse movement generates 100-1000+ HID reports per second (depending on polling rate). Including mice would cause constant false positives. Since BadUSB attacks use keyboard emulation (not mouse emulation), excluding mouse protocol devices (protocol 2) is safe.

### Why the GPIO-based `usb_detector` module is unused

The project originally used GPIO-level D+/D- detection (`usb_detector.c`) to detect physical USB connections. This was replaced by TinyUSB-based enumeration which provides much richer information (VID, PID, device class, string descriptors). The `usb_detector` module remains in the source tree but is not compiled or called. See [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) for its original documentation.

### Why USB CDC stdio is disabled

The RP2040's native USB port (port 0) is used in host mode to enumerate plugged-in devices. It cannot simultaneously act as a CDC serial device. Debug output goes through UART instead (enabled by `pico_enable_stdio_uart`).

### Capacity limits

All arrays are sized to 4 devices (`MAX_DEVICES`, `MAX_HID_DEVICES`, `MAX_TRACKED_DEVICES`, `CFG_TUH_DEVICE_MAX`). This matches the TinyUSB host stack limit and is sufficient for the single-port use case. Hub support is enabled only for detection/warning, not to enumerate downstream devices.

## File Metrics

| File | Lines | Module |
|------|-------|--------|
| `main.c` | 444 | Application |
| `src/usb_host.c` | 448 | USB Host |
| `src/usb_detector.c` | 240 | (Legacy) GPIO Detector |
| `src/threat_analyzer.c` | 213 | Threat Analyzer |
| `src/oled_graphics.c` | 197 | OLED Graphics |
| `src/oled_driver.c` | 195 | OLED Driver |
| `src/oled_font.c` | 133 | OLED Font |
| `src/oled_display.c` | 133 | OLED Display |
| `src/hid_monitor.c` | 113 | HID Monitor |
| `src/oled_text.c` | 84 | OLED Text |
| `src/oled_i2c.c` | 72 | OLED I2C |
| **Total** | **~2,800** | |
