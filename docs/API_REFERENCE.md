# PlugSafe — API Reference

Complete reference for all public types, constants, and functions across every module.

## Table of Contents

- [OLED I2C (`oled_i2c.h`)](#oled-i2c)
- [OLED Config (`oled_config.h`)](#oled-config)
- [OLED Driver (`oled_driver.h`)](#oled-driver)
- [OLED Display (`oled_display.h`)](#oled-display)
- [OLED Graphics (`oled_graphics.h`)](#oled-graphics)
- [OLED Text (`oled_text.h`)](#oled-text)
- [OLED Font (`oled_font.h`)](#oled-font)
- [USB Host (`usb_host.h`)](#usb-host)
- [HID Monitor (`hid_monitor.h`)](#hid-monitor)
- [Threat Analyzer (`threat_analyzer.h`)](#threat-analyzer)
- [USB Detector (`usb_detector.h`) — Legacy](#usb-detector-legacy)
- [TinyUSB Configuration (`tusb_config.h`)](#tinyusb-configuration)

---

## OLED I2C

**Header:** `include/oled_i2c.h`
**Source:** `src/oled_i2c.c`
**Purpose:** Low-level I2C communication with the OLED controller.

### Constants

| Name | Value | Description |
|------|-------|-------------|
| `OLED_I2C_CTRL_CMD` | `0x00` | Control byte for sending commands |
| `OLED_I2C_CTRL_DATA` | `0x40` | Control byte for sending pixel data |

### Struct: `oled_i2c_t`

```c
typedef struct {
    i2c_inst_t *i2c;    /* Pico I2C instance (i2c0 or i2c1) */
    uint sda_pin;        /* GPIO pin for SDA */
    uint scl_pin;        /* GPIO pin for SCL */
    uint baudrate;       /* Baud rate in Hz (typically 400000) */
    uint8_t address;     /* 7-bit I2C address (typically 0x3C) */
} oled_i2c_t;
```

### Functions

#### `oled_i2c_init`
```c
bool oled_i2c_init(oled_i2c_t *i2c);
```
Initializes the I2C peripheral, configures GPIO pins as I2C with internal pull-ups.
- **Returns:** `true` on success, `false` on failure.

#### `oled_i2c_write_cmd`
```c
bool oled_i2c_write_cmd(oled_i2c_t *i2c, const uint8_t *cmds, size_t len);
```
Sends one or more command bytes to the OLED controller (control byte `0x00`).
- `cmds` — Array of command bytes.
- `len` — Number of bytes.

#### `oled_i2c_write_data`
```c
bool oled_i2c_write_data(oled_i2c_t *i2c, const uint8_t *data, size_t len);
```
Sends pixel data to the OLED controller (control byte `0x40`).
- `data` — Array of pixel data bytes.
- `len` — Number of bytes.

#### `oled_i2c_write_raw`
```c
bool oled_i2c_write_raw(oled_i2c_t *i2c, uint8_t ctrl_byte,
                        const uint8_t *data, size_t len);
```
Low-level I2C write. Prepends `ctrl_byte` before `data` and sends via `i2c_write_blocking()`.
- `ctrl_byte` — Control byte (`0x00` for commands, `0x40` for data).

---

## OLED Config

**Header:** `include/oled_config.h`
**Purpose:** Central configuration constants and types for all OLED modules. No source file.

### Constants

| Name | Value | Description |
|------|-------|-------------|
| `OLED_WIDTH` | `128` | Display width in pixels |
| `OLED_HEIGHT` | `64` | Display height in pixels |
| `OLED_PAGES` | `8` | Number of 8-pixel-high pages (64/8) |
| `OLED_BUFFER_SIZE` | `1024` | Framebuffer size in bytes (128 x 64 / 8) |
| `OLED_I2C_ADDRESS_DEFAULT` | `0x3C` | Default I2C address |
| `OLED_I2C_ADDRESS_ALT` | `0x3D` | Alternate I2C address |
| `OLED_I2C_BAUDRATE_DEFAULT` | `400000` | Default I2C clock speed (400 kHz) |
| `OLED_I2C_CTRL_CMD` | `0x00` | Command control byte |
| `OLED_I2C_CTRL_DATA` | `0x40` | Data control byte |
| `OLED_I2C_CTRL_CMD_DATA` | `0x80` | Single command + data control byte |
| `OLED_I2C_CTRL_CMDS` | `0xC0` | Multiple commands control byte |
| `OLED_ENABLE_GRAPHICS` | `1` | Enable graphics primitives |
| `OLED_ENABLE_TEXT` | `1` | Enable text rendering |
| `OLED_ENABLE_FONTS` | `1` | Enable font data |

### Enum: `oled_display_type_e`

```c
typedef enum {
    OLED_DISPLAY_SSD1306,   /* SSD1306 controller (most common) */
    OLED_DISPLAY_SH1106     /* SH1106 controller (auto +2 column offset) */
} oled_display_type_e;
```

### Enum: `oled_status_e`

```c
typedef enum {
    OLED_OK        = 0,   /* Success */
    OLED_ERR_INIT  = 1,   /* Initialization error */
    OLED_ERR_I2C   = 2,   /* I2C communication error */
    OLED_ERR_INVALID = 3, /* Invalid parameter */
    OLED_ERR_NOMEM = 4,   /* Memory allocation error */
    OLED_ERR_HW    = 5    /* Hardware error */
} oled_status_e;
```

---

## OLED Driver

**Header:** `include/oled_driver.h`
**Source:** `src/oled_driver.c`
**Purpose:** SSD1306/SH1106 controller initialization sequences and hardware commands.

### Struct: `oled_driver_t`

```c
typedef struct {
    oled_i2c_t *i2c;            /* I2C instance */
    oled_display_type_e type;   /* Controller type */
    uint8_t width;              /* Display width (128) */
    uint8_t height;             /* Display height (64) */
    bool is_on;                 /* Display power state */
    uint8_t contrast;           /* Current contrast level */
    uint8_t page_start;         /* Starting page address */
} oled_driver_t;
```

### Enum: `ssd1306_cmd_e`

```c
typedef enum {
    SSD1306_CMD_SETCONTRAST   = 0x81,
    SSD1306_CMD_DISPLAYRAM    = 0xA4,   /* Resume from RAM content */
    SSD1306_CMD_DISPLAYNORMAL = 0xA6,   /* Normal display (not inverted) */
    SSD1306_CMD_DISPLAYINVERT = 0xA7,   /* Inverted display */
    SSD1306_CMD_DISPLAYOFF    = 0xAE,   /* Power off */
    SSD1306_CMD_DISPLAYON     = 0xAF,   /* Power on */
    SSD1306_CMD_SETPAGE       = 0xB0,   /* Set page address (OR with page number) */
    SSD1306_CMD_SETLOWCOL     = 0x00,   /* Set lower column nibble */
    SSD1306_CMD_SETHIGHCOL    = 0x10    /* Set upper column nibble */
} ssd1306_cmd_e;
```

### Functions

#### `oled_driver_init`
```c
bool oled_driver_init(oled_driver_t *driver,
                      oled_display_type_e type,
                      oled_i2c_t *i2c);
```
Sends the full initialization sequence for the specified controller type (charge pump, memory mode, segment remap, COM direction, contrast, etc.). Waits 100ms for the display to stabilize.
- `type` — `OLED_DISPLAY_SSD1306` or `OLED_DISPLAY_SH1106`.

#### `oled_driver_set_page`
```c
bool oled_driver_set_page(oled_driver_t *driver, uint8_t page);
```
Sets the current page address (0-7). Sends command `0xB0 | page`.

#### `oled_driver_set_column`
```c
bool oled_driver_set_column(oled_driver_t *driver, uint8_t col);
```
Sets the current column address (0-127). For SH1106, applies a +2 offset automatically.

#### `oled_driver_write_pixel_data`
```c
bool oled_driver_write_pixel_data(oled_driver_t *driver,
                                  const uint8_t *data, size_t len);
```
Writes pixel data at the current page/column position.

#### `oled_driver_power_on` / `oled_driver_power_off`
```c
bool oled_driver_power_on(oled_driver_t *driver);
bool oled_driver_power_off(oled_driver_t *driver);
```
Sends display ON (`0xAF`) or OFF (`0xAE`) command.

#### `oled_driver_set_contrast`
```c
bool oled_driver_set_contrast(oled_driver_t *driver, uint8_t contrast);
```
Sets display contrast (0-255). Default is `0x7F` after init.

#### `oled_driver_set_invert`
```c
bool oled_driver_set_invert(oled_driver_t *driver, bool invert);
```
Sets normal (`0xA6`) or inverted (`0xA7`) display mode.

---

## OLED Display

**Header:** `include/oled_display.h`
**Source:** `src/oled_display.c`
**Purpose:** Framebuffer management. Maintains a 1024-byte in-memory buffer and flushes it to hardware.

### Struct: `oled_display_t`

```c
typedef struct {
    oled_driver_t *driver;  /* Underlying hardware driver */
    uint8_t *buffer;        /* Framebuffer (malloc'd, 1024 bytes) */
    uint8_t width;          /* Display width (128) */
    uint8_t height;         /* Display height (64) */
    bool dirty;             /* True if buffer has been modified since last flush */
} oled_display_t;
```

### Functions

#### `oled_display_init`
```c
bool oled_display_init(oled_display_t *display, oled_driver_t *driver);
```
Allocates 1024-byte framebuffer via `malloc()`. Clears buffer to zero.
- **Returns:** `false` if memory allocation fails.

#### `oled_display_deinit`
```c
void oled_display_deinit(oled_display_t *display);
```
Frees the framebuffer memory.

#### `oled_display_clear`
```c
void oled_display_clear(oled_display_t *display);
```
Sets all framebuffer bytes to `0x00` (all pixels off). Sets dirty flag.

#### `oled_display_invert`
```c
void oled_display_invert(oled_display_t *display, bool invert);
```
Bitwise NOTs the entire framebuffer (inverts all pixels). Sets dirty flag.

#### `oled_display_flush`
```c
bool oled_display_flush(oled_display_t *display);
```
Writes the entire framebuffer to the OLED hardware. Iterates over 8 pages, setting page/column for each and writing 128 bytes per page.

#### `oled_display_get_buffer`
```c
uint8_t *oled_display_get_buffer(oled_display_t *display);
```
Returns a pointer to the raw framebuffer for direct manipulation.

#### `oled_display_get_buffer_size`
```c
size_t oled_display_get_buffer_size(oled_display_t *display);
```
Returns `OLED_BUFFER_SIZE` (1024).

#### `oled_display_get_dimensions`
```c
void oled_display_get_dimensions(oled_display_t *display,
                                 uint8_t *width, uint8_t *height);
```
Writes display dimensions into the provided pointers.

---

## OLED Graphics

**Header:** `include/oled_graphics.h`
**Source:** `src/oled_graphics.c`
**Purpose:** Pixel-level and shape drawing into the framebuffer.

### Functions

#### `oled_draw_pixel`
```c
void oled_draw_pixel(oled_display_t *display, int x, int y, bool on);
```
Sets or clears a single pixel. Computes `page = y / 8`, `bit = y % 8`, `index = page * 128 + x`. Bounds-checked.

#### `oled_get_pixel`
```c
bool oled_get_pixel(oled_display_t *display, int x, int y);
```
Returns the state of a pixel at (x, y). Returns `false` if out of bounds.

#### `oled_draw_hline`
```c
void oled_draw_hline(oled_display_t *display, int x, int y, int len, bool on);
```
Draws a horizontal line starting at (x, y) extending `len` pixels to the right.

#### `oled_draw_vline`
```c
void oled_draw_vline(oled_display_t *display, int x, int y, int len, bool on);
```
Draws a vertical line starting at (x, y) extending `len` pixels downward.

#### `oled_draw_line`
```c
void oled_draw_line(oled_display_t *display, int x0, int y0, int x1, int y1, bool on);
```
Draws an arbitrary line between two points using **Bresenham's line algorithm**.

#### `oled_draw_rect`
```c
void oled_draw_rect(oled_display_t *display, int x, int y, int w, int h,
                    bool fill, bool on);
```
Draws a rectangle. If `fill` is true, fills the interior. Otherwise draws outline only.

#### `oled_draw_circle`
```c
void oled_draw_circle(oled_display_t *display, int cx, int cy, int r,
                      bool fill, bool on);
```
Draws a circle using the **midpoint circle algorithm**. If `fill` is true, fills the interior with horizontal lines.

#### `oled_draw_bitmap`
```c
void oled_draw_bitmap(oled_display_t *display, int x, int y,
                      const uint8_t *bitmap, int w, int h);
```
Draws a 1-bit bitmap at position (x, y). Bitmap is stored column-major: each byte represents 8 vertical pixels.

---

## OLED Text

**Header:** `include/oled_text.h`
**Source:** `src/oled_text.c`
**Purpose:** Character and string rendering using bitmap fonts.

### Struct: `oled_font_t`

```c
typedef struct {
    const uint8_t *data;    /* Pointer to bitmap font data */
    uint16_t width;         /* Total data width in pixels */
    uint8_t height;         /* Character height in pixels */
    uint8_t char_width;     /* Width of each character cell in pixels */
    char start_char;        /* First character in font (typically 0x20 = space) */
    char end_char;          /* Last character in font (typically 0x7E = ~) */
} oled_font_t;
```

### Functions

#### `oled_draw_char`
```c
int oled_draw_char(oled_display_t *display, int x, int y,
                   char c, const oled_font_t *font, bool on);
```
Renders a single character at position (x, y).
- `on` — `true` for white-on-black, `false` for black-on-white.
- **Returns:** Character width in pixels (for cursor advancement).

#### `oled_draw_string`
```c
int oled_draw_string(oled_display_t *display, int x, int y,
                     const char *str, const oled_font_t *font, bool on);
```
Renders a null-terminated string. Characters are spaced by `char_width + 1` pixels.
- **Returns:** Total width of the rendered string in pixels.

#### `oled_measure_string`
```c
int oled_measure_string(const char *str, const oled_font_t *font);
```
Calculates the pixel width of a string without drawing it. Useful for centering text.

---

## OLED Font

**Header:** `include/oled_font.h`
**Source:** `src/oled_font.c`
**Purpose:** Built-in bitmap font data.

### Available Font

The built-in font is a **5x7 pixel** ASCII bitmap covering characters `0x20` (space) through `0x7E` (`~`) — 95 characters, 475 bytes total. Each character is 5 columns of 7-bit data stored column-major.

### Functions

#### `oled_get_font_5x7`
```c
const oled_font_t *oled_get_font_5x7(void);
```
Returns a pointer to the built-in 5x7 font.

#### `oled_get_font_8x8`
```c
const oled_font_t *oled_get_font_8x8(void);
```
**Stub** — currently returns the 5x7 font. An 8x8 font has not been implemented yet.

---

## USB Host

**Header:** `include/usb_host.h`
**Source:** `src/usb_host.c`
**Purpose:** TinyUSB host integration, device enumeration, descriptor and string parsing, HID report forwarding.

### Struct: `usb_device_info_t`

```c
typedef struct {
    uint8_t dev_addr;           /* TinyUSB device address (1-4) */
    uint8_t instance;           /* HID interface instance (0-3) */
    uint16_t vid;               /* Vendor ID */
    uint16_t pid;               /* Product ID */
    uint8_t usb_class;          /* USB Device Class code */
    uint8_t subclass;           /* USB Device Subclass */
    uint8_t protocol;           /* USB Device Protocol */
    char manufacturer[64];      /* Manufacturer string (UTF-8) */
    char product[64];           /* Product string (UTF-8) */
    char serial[64];            /* Serial number string (UTF-8) */
    bool is_hid;                /* True if device has a HID interface */
    uint8_t hid_protocol;       /* HID protocol: 0=None, 1=Keyboard, 2=Mouse */
    bool is_mounted;            /* True if device is currently mounted */
    bool descriptor_ready;      /* True once VID/PID/Class have been read */
    bool strings_ready;         /* True once manufacturer/product/serial have been read */
    uint64_t connected_time_ms; /* Timestamp when device was connected */
} usb_device_info_t;
```

### Functions

#### `usb_host_init`
```c
bool usb_host_init(void);
```
Initializes TinyUSB in host mode on port 0 (`BOARD_TUH_RHPORT`). Calls `tusb_init()`.
- **Returns:** `true` on success.

#### `usb_host_task`
```c
void usb_host_task(void);
```
Processes TinyUSB events. Must be called regularly from the main loop (every 10ms recommended). Internally calls `tuh_task()`.

#### `usb_get_device_info`
```c
usb_device_info_t* usb_get_device_info(uint8_t dev_addr);
```
Returns device info for a given TinyUSB device address. Returns `NULL` if not found.

#### `usb_get_device_count`
```c
uint8_t usb_get_device_count(void);
```
Returns the number of currently mounted USB devices.

#### `usb_get_device_at_index`
```c
usb_device_info_t* usb_get_device_at_index(uint8_t index);
```
Returns device info by index (0 to `count - 1`). Returns `NULL` if index is out of range.

#### `usb_is_device_mounted`
```c
bool usb_is_device_mounted(uint8_t dev_addr);
```
Returns `true` if the device at the given address is currently mounted.

#### `usb_is_hub_connected`
```c
bool usb_is_hub_connected(void);
```
Returns `true` if a USB hub (class `0x09`) is currently connected. Used to trigger the hub warning screen.

### TinyUSB Callbacks (implemented in `usb_host.c`)

These are not part of the public API but are documented here for reference. They are called by TinyUSB internally:

| Callback | Trigger | Actions |
|----------|---------|---------|
| `tuh_mount_cb(daddr)` | Device mounted | Fetch descriptors, parse strings (UTF-16LE to UTF-8), call `threat_add_device()` |
| `tuh_umount_cb(daddr)` | Device unmounted | Call `threat_remove_device()`, `hid_monitor_remove_device()`, clear slot |
| `tuh_hid_mount_cb(dev_addr, instance, ...)` | HID interface mounted | Record HID protocol, call `hid_monitor_add_device()` (non-mouse only), `threat_update_device_info()`, start reports |
| `tuh_hid_umount_cb(dev_addr, instance)` | HID interface unmounted | Log only |
| `tuh_hid_report_received_cb(dev_addr, instance, report, len)` | HID report received | Forward to `hid_monitor_report()` and `threat_update_hid_activity()` (non-mouse only), re-request next report |

---

## HID Monitor

**Header:** `include/hid_monitor.h`
**Source:** `src/hid_monitor.c`
**Purpose:** Tracks per-device HID report rate using a 1-second sliding window.

### Constants

| Name | Value | Description |
|------|-------|-------------|
| `HID_KEYSTROKE_THRESHOLD_HZ` | `50` | Reports/sec above which a device is flagged |
| `KEYSTROKE_RATE_WINDOW_MS` | `1000` | Measurement window size (1 second) |
| `MAX_HID_DEVICES` | `4` | Maximum simultaneously monitored HID devices |

### Struct: `hid_monitor_t`

```c
typedef struct {
    uint8_t dev_addr;               /* Device address being monitored */
    uint32_t total_reports;         /* Total HID reports received (lifetime) */
    uint32_t reports_this_second;   /* Reports in current 1-second window */
    uint64_t last_window_start_ms;  /* Start timestamp of current window */
    uint32_t peak_rate_hz;          /* Highest rate ever seen for this device */
    uint32_t current_rate_hz;       /* Rate from the most recent completed window */
    bool is_monitoring;             /* True if this slot is active */
} hid_monitor_t;
```

### Functions

#### `hid_monitor_init`
```c
void hid_monitor_init(void);
```
Zeroes all monitor slots. Called at startup.

#### `hid_monitor_add_device`
```c
void hid_monitor_add_device(uint8_t dev_addr);
```
Registers a device for monitoring. Finds a free slot, initializes counters, sets `is_monitoring = true`.

#### `hid_monitor_report`
```c
void hid_monitor_report(uint8_t dev_addr, const uint8_t *report, uint16_t len);
```
Core rate calculation. Increments `reports_this_second`. When the 1-second window elapses, computes `current_rate_hz = (reports * 1000) / elapsed_ms`, updates `peak_rate_hz`, resets the window. Logs an alert if rate exceeds `HID_KEYSTROKE_THRESHOLD_HZ`.

#### `hid_get_keystroke_rate`
```c
uint32_t hid_get_keystroke_rate(uint8_t dev_addr);
```
Returns `current_rate_hz` for the given device. Returns `0` if device is not being monitored.

#### `hid_is_spammy`
```c
bool hid_is_spammy(uint8_t dev_addr);
```
Returns `true` if `current_rate_hz > HID_KEYSTROKE_THRESHOLD_HZ` (50).

#### `hid_monitor_remove_device`
```c
void hid_monitor_remove_device(uint8_t dev_addr);
```
Clears the monitoring slot. Logs the peak rate seen during the device's session.

#### `hid_get_monitor_stats`
```c
hid_monitor_t* hid_get_monitor_stats(uint8_t dev_addr);
```
Returns a pointer to the `hid_monitor_t` struct for a device. Returns `NULL` if not found.

---

## Threat Analyzer

**Header:** `include/threat_analyzer.h`
**Source:** `src/threat_analyzer.c`
**Purpose:** Classifies USB devices into threat levels and tracks device threat state over time.

### Constants

| Name | Value | Description |
|------|-------|-------------|
| `HID_KEYSTROKE_THRESHOLD_HZ` | `50` | Keystroke rate to flag as malicious |
| `RATE_NORMAL_MAX_HZ` | `30` | Normal human typing ceiling (informational) |
| `RATE_SUSPICIOUS_MIN_HZ` | `50` | Suspicion threshold (same as `HID_KEYSTROKE_THRESHOLD_HZ`) |

### Enum: `threat_level_e`

```c
typedef enum {
    THREAT_SAFE               = 0,  /* Non-HID device — automatically safe */
    THREAT_POTENTIALLY_UNSAFE = 1,  /* HID keyboard/unknown — requires monitoring */
    THREAT_MALICIOUS          = 2   /* HID with attack signature (>50 keys/sec) */
} threat_level_e;
```

### Struct: `device_threat_t`

```c
typedef struct {
    uint8_t dev_addr;                   /* TinyUSB device address */
    usb_device_info_t device;           /* Snapshot of device info at classification time */
    threat_level_e threat_level;        /* Current threat classification */
    uint32_t hid_report_count;          /* Total HID reports received */
    uint32_t hid_reports_per_sec;       /* Live keystroke rate (from hid_monitor) */
    bool is_active;                     /* True if this tracking slot is in use */
} device_threat_t;
```

### Functions

#### `threat_analyzer_init`
```c
void threat_analyzer_init(void);
```
Zeroes the tracking array. Called at startup.

#### `threat_analyze_device`
```c
threat_level_e threat_analyze_device(const usb_device_info_t *info);
```
Static classification based on device descriptors. Does not modify any state.
- Non-HID → `THREAT_SAFE`
- HID Mouse (protocol 2) → `THREAT_SAFE`
- HID Keyboard (protocol 1) or Unknown HID (protocol 0) → `THREAT_POTENTIALLY_UNSAFE`

#### `threat_add_device`
```c
void threat_add_device(const usb_device_info_t *dev_info);
```
Copies device info into a tracking slot and runs initial classification. Called from `tuh_mount_cb()`.

#### `threat_update_device_info`
```c
void threat_update_device_info(const usb_device_info_t *dev_info);
```
Updates the stored device snapshot and re-classifies. Only escalates — never downgrades the threat level. Called from `tuh_hid_mount_cb()` when HID info becomes available after initial mount.

#### `threat_update_hid_activity`
```c
void threat_update_hid_activity(uint8_t dev_addr, uint16_t report_len);
```
Core runtime analysis. Reads the windowed rate from `hid_get_keystroke_rate()`. If rate > 50 Hz, escalates to `THREAT_MALICIOUS` (sticky). Logs a detailed threat warning with device name, VID/PID, and rate. Called on every HID report.

#### `threat_get_current_level`
```c
threat_level_e threat_get_current_level(uint8_t dev_addr);
```
Returns the current threat level for a device. Returns `THREAT_SAFE` if device is not tracked.

#### `threat_get_device_status`
```c
device_threat_t* threat_get_device_status(uint8_t dev_addr);
```
Returns a pointer to the `device_threat_t` for a device. Returns `NULL` if not found. Does not auto-create.

#### `threat_get_device_at_index`
```c
device_threat_t* threat_get_device_at_index(uint8_t index);
```
Returns the threat status of the device at the given index (0 to 3). Returns `NULL` if the slot is inactive.

#### `threat_is_hid_spammy`
```c
bool threat_is_hid_spammy(uint8_t dev_addr);
```
Returns `true` if the device's threat level is `THREAT_MALICIOUS`.

#### `threat_remove_device`
```c
void threat_remove_device(uint8_t dev_addr);
```
Clears the tracking slot. Called from `tuh_umount_cb()`.

---

## USB Detector (Legacy)

**Header:** `include/usb_detector.h`
**Source:** `src/usb_detector.c`
**Purpose:** GPIO-based USB device detection via D+/D- pins with debouncing and LED feedback.

> **Note:** This module is **not compiled or used** in the current firmware. It was part of an earlier iteration before TinyUSB-based enumeration was implemented. The source files remain in the tree for reference. See [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) for its original documentation.

### Constants

| Name | Value | Description |
|------|-------|-------------|
| `USB_DETECTOR_LED_PIN` | `25` | Built-in Pico LED |
| `USB_DETECTOR_VBUS_PIN` | `40` | USB 5V detection |
| `USB_DETECTOR_DP_PIN` | `0` | USB D+ (green wire) |
| `USB_DETECTOR_DM_PIN` | `1` | USB D- (white wire) |
| `USB_DETECTOR_SEARCH_BLINK_PERIOD_MS` | `1000` | LED blink period when searching |
| `USB_DETECTOR_DEVICE_BLINK_PERIOD_MS` | `400` | LED blink period when device detected |
| `USB_DETECTOR_VBUS_DEBOUNCE_MS` | `50` | Debounce delay |
| `USB_DETECTOR_UPDATE_INTERVAL_MS` | `20` | Recommended polling interval |

### Enum: `usb_detector_state_t`

```c
typedef enum {
    USB_DETECTOR_STATE_SEARCHING = 0,   /* No device detected */
    USB_DETECTOR_STATE_DETECTED  = 1    /* Device detected */
} usb_detector_state_t;
```

### Functions

#### `usb_detector_init`
```c
void usb_detector_init(void);
```
Configures GP25 as output, GP0/GP1 as inputs with pull-down. Sets initial state to `SEARCHING`.

#### `usb_detector_update`
```c
void usb_detector_update(void);
```
State machine: reads D+/D-, debounces, transitions between SEARCHING/DETECTED, controls LED blink.

#### `usb_detector_get_state`
```c
usb_detector_state_t usb_detector_get_state(void);
```
Returns the current state.

#### `usb_detector_is_device_connected`
```c
bool usb_detector_is_device_connected(void);
```
Returns `true` if state is `DETECTED`.

#### `usb_detector_get_state_duration_ms`
```c
uint32_t usb_detector_get_state_duration_ms(void);
```
Returns milliseconds since the last state transition.

#### `usb_detector_set_led`
```c
void usb_detector_set_led(uint8_t state);
```
Manual LED control. `1` = ON, `0` = OFF. Disables automatic blinking.

---

## TinyUSB Configuration

**File:** `tusb_config.h` (project root)
**Purpose:** Compile-time configuration for the TinyUSB stack.

### Common Settings

| Define | Value | Description |
|--------|-------|-------------|
| `CFG_TUSB_OS` | `OPT_OS_NONE` | Bare metal, no RTOS |
| `CFG_TUSB_DEBUG` | `1` | Debug level: error messages only |
| `CFG_TUH_MEM_ALIGN` | `__attribute__((aligned(4)))` | 4-byte DMA alignment |

### Host Settings

| Define | Value | Description |
|--------|-------|-------------|
| `CFG_TUH_ENABLED` | `1` | Enable USB host stack |
| `BOARD_TUH_RHPORT` | `0` | Native USB port (micro-USB via OTG) |
| `BOARD_TUH_MAX_SPEED` | `OPT_MODE_DEFAULT_SPEED` | Full-speed (12 Mbps) |

### Driver Settings

| Define | Value | Description |
|--------|-------|-------------|
| `CFG_TUH_ENUMERATION_BUFSIZE` | `256` | Descriptor buffer size |
| `CFG_TUH_DEVICE_MAX` | `4` | Maximum connected devices |
| `CFG_TUH_HUB` | `1` | Hub support (detect + warn only) |
| `CFG_TUH_HID` | `12` | Max HID interfaces (3 per device x 4 devices) |
| `CFG_TUH_CDC` | `0` | CDC support disabled |
| `CFG_TUH_MSC` | `0` | Mass storage support disabled |
| `CFG_TUH_VENDOR` | `0` | Vendor class support disabled |
| `CFG_TUH_HID_EPIN_BUFSIZE` | `64` | HID IN endpoint buffer |
| `CFG_TUH_HID_EPOUT_BUFSIZE` | `64` | HID OUT endpoint buffer |
