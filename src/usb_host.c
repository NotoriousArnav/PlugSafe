/*
 * PlugSafe USB Host Layer Implementation
 * Real TinyUSB integration for device enumeration and HID monitoring
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "usb_host.h"
#include "threat_analyzer.h"
#include "hid_monitor.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

/* Language ID for string descriptor requests (English US) */
#define LANGUAGE_ID 0x0409

/* Maximum number of tracked devices */
#define MAX_DEVICES 4

/* ============================================================================
 * USB TRANSFER BUFFERS (DMA-aligned for USB controller)
 * ============================================================================ */

/* Descriptor buffer for USB transfers — must be in USB/DMA memory section
 * and properly aligned for the USB controller. */
CFG_TUH_MEM_SECTION static struct {
    TUH_EPBUF_TYPE_DEF(tusb_desc_device_t, device);
    TUH_EPBUF_DEF(buf, 256);
} _desc;

/* ============================================================================
 * DEVICE TRACKING & STATE MANAGEMENT
 * ============================================================================ */

/* Connected USB devices */
static usb_device_info_t g_usb_devices[MAX_DEVICES];

/* Track if a USB hub is connected (warning flag) */
static bool g_hub_connected = false;

/* ============================================================================
 * UTF-16 TO UTF-8 CONVERSION HELPERS
 * (Adapted from TinyUSB device_info example)
 * ============================================================================ */

/**
 * @brief Convert UTF-16LE to UTF-8
 */
static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len,
                                     uint8_t *utf8, size_t utf8_len) {
    (void) utf8_len;
    for (size_t i = 0; i < utf16_len; i++) {
        uint16_t chr = utf16[i];
        if (chr < 0x80) {
            *utf8++ = (uint8_t)(chr & 0xff);
        } else if (chr < 0x800) {
            *utf8++ = (uint8_t)(0xC0 | (chr >> 6 & 0x1F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
        } else {
            *utf8++ = (uint8_t)(0xE0 | (chr >> 12 & 0x0F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 6 & 0x3F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
        }
    }
}

/**
 * @brief Count how many bytes a UTF-16LE string needs in UTF-8
 */
static int _count_utf8_bytes(const uint16_t *buf, size_t len) {
    size_t total_bytes = 0;
    for (size_t i = 0; i < len; i++) {
        uint16_t chr = buf[i];
        if (chr < 0x80) {
            total_bytes += 1;
        } else if (chr < 0x800) {
            total_bytes += 2;
        } else {
            total_bytes += 3;
        }
    }
    return (int)total_bytes;
}

/**
 * @brief Parse a USB string descriptor (UTF-16LE) into a UTF-8 C string.
 * @param desc_str  Raw USB string descriptor buffer (first word is length+type)
 * @param buf_len   Total buffer size in uint16_t units
 * @param out       Destination UTF-8 buffer
 * @param out_len   Size of destination buffer
 */
static void _parse_string_descriptor(const uint8_t *desc_str, size_t buf_len,
                                     char *out, size_t out_len) {
    /* USB string descriptor format:
     * Byte 0: bLength (total length including this byte)
     * Byte 1: bDescriptorType (0x03 = string)
     * Byte 2+: UTF-16LE characters
     */
    const uint16_t *temp_buf = (const uint16_t *)desc_str;
    if (buf_len < 1 || (temp_buf[0] & 0xFF) < 4) {
        /* Empty or invalid descriptor */
        out[0] = '\0';
        return;
    }

    size_t utf16_len = ((temp_buf[0] & 0xFF) - 2) / sizeof(uint16_t);
    if (utf16_len == 0) {
        out[0] = '\0';
        return;
    }

    int utf8_len = _count_utf8_bytes(temp_buf + 1, utf16_len);
    if ((size_t)utf8_len >= out_len) {
        utf8_len = (int)(out_len - 1);
    }

    _convert_utf16le_to_utf8(temp_buf + 1, utf16_len, (uint8_t *)out, out_len);
    out[utf8_len] = '\0';
}

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * @brief Find an empty slot in the device array
 * @return Pointer to a free slot, or NULL if full
 */
static usb_device_info_t *_find_free_slot(void) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!g_usb_devices[i].is_mounted) {
            return &g_usb_devices[i];
        }
    }
    return NULL;
}

/**
 * @brief Find a device by its TinyUSB device address
 * @return Pointer to the device, or NULL
 */
static usb_device_info_t *_find_device(uint8_t dev_addr) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_usb_devices[i].dev_addr == dev_addr && g_usb_devices[i].is_mounted) {
            return &g_usb_devices[i];
        }
    }
    return NULL;
}

/* ============================================================================
 * PUBLIC API FUNCTIONS
 * ============================================================================ */

bool usb_host_init(void) {
    printf("[USB] Initializing USB Host Stack...\n");

    /* Clear device array */
    memset(g_usb_devices, 0, sizeof(g_usb_devices));
    g_hub_connected = false;

    /* Initialize TinyUSB host stack on native USB port 0 */
    tusb_rhport_init_t host_init = {
        .role = TUSB_ROLE_HOST,
        .speed = TUSB_SPEED_AUTO
    };
    if (!tusb_init(BOARD_TUH_RHPORT, &host_init)) {
        printf("[USB] ERROR: tusb_init() failed!\n");
        return false;
    }

    printf("[USB] TinyUSB host initialized on port %d\n", BOARD_TUH_RHPORT);
    printf("[USB] Ready for device enumeration\n");

    return true;
}

void usb_host_task(void) {
    /* Process TinyUSB host events (enumeration, callbacks, etc.) */
    tuh_task();
}

usb_device_info_t *usb_get_device_info(uint8_t dev_addr) {
    return _find_device(dev_addr);
}

uint8_t usb_get_device_count(void) {
    uint8_t count = 0;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_usb_devices[i].is_mounted) {
            count++;
        }
    }
    return count;
}

usb_device_info_t *usb_get_device_at_index(uint8_t index) {
    uint8_t count = 0;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_usb_devices[i].is_mounted) {
            if (count == index) {
                return &g_usb_devices[i];
            }
            count++;
        }
    }
    return NULL;
}

bool usb_is_device_mounted(uint8_t dev_addr) {
    return (_find_device(dev_addr) != NULL);
}

bool usb_is_hub_connected(void) {
    return g_hub_connected;
}

/* ============================================================================
 * TinyUSB HOST CALLBACKS
 * ============================================================================ */

/**
 * @brief Called by TinyUSB when a device is mounted (enumerated).
 *
 * We grab the device descriptor synchronously to get VID, PID, class,
 * and then fetch string descriptors for manufacturer/product/serial.
 */
void tuh_mount_cb(uint8_t daddr) {
    printf("[USB] Device mounted at address %d\n", daddr);

    usb_device_info_t *dev = _find_free_slot();
    if (!dev) {
        printf("[USB] ERROR: No free slot for device %d\n", daddr);
        return;
    }

    memset(dev, 0, sizeof(*dev));
    dev->dev_addr = daddr;
    dev->is_mounted = true;
    dev->connected_time_ms = to_ms_since_boot(get_absolute_time());

    /* ---- Device descriptor (synchronous) ---- */
    uint8_t xfer_result = tuh_descriptor_get_device_sync(daddr, &_desc.device, 18);
    if (XFER_RESULT_SUCCESS == xfer_result) {
        dev->vid = _desc.device.idVendor;
        dev->pid = _desc.device.idProduct;
        dev->usb_class = _desc.device.bDeviceClass;
        dev->subclass = _desc.device.bDeviceSubClass;
        dev->protocol = _desc.device.bDeviceProtocol;
        dev->descriptor_ready = true;

        printf("[USB] VID: 0x%04X  PID: 0x%04X  Class: 0x%02X\n",
               dev->vid, dev->pid, dev->usb_class);

        /* Check for hub (class 0x09) */
        if (dev->usb_class == 0x09) {
            g_hub_connected = true;
            printf("[USB] WARNING: USB Hub detected!\n");
        }
    } else {
        printf("[USB] WARNING: Failed to get device descriptor (result=%d)\n", xfer_result);
        /* Set defaults */
        snprintf(dev->manufacturer, sizeof(dev->manufacturer), "Unknown");
        snprintf(dev->product, sizeof(dev->product), "USB Device");
        snprintf(dev->serial, sizeof(dev->serial), "N/A");
        dev->descriptor_ready = false;
    }

    /* ---- String descriptors (synchronous) ---- */
    if (dev->descriptor_ready) {
        /* Manufacturer string */
        if (_desc.device.iManufacturer != 0) {
            xfer_result = tuh_descriptor_get_manufacturer_string_sync(
                daddr, LANGUAGE_ID, _desc.buf, sizeof(_desc.buf));
            if (XFER_RESULT_SUCCESS == xfer_result) {
                _parse_string_descriptor(_desc.buf, sizeof(_desc.buf) / 2,
                                         dev->manufacturer, sizeof(dev->manufacturer));
            }
        }
        if (dev->manufacturer[0] == '\0') {
            snprintf(dev->manufacturer, sizeof(dev->manufacturer), "Unknown");
        }

        /* Product string */
        if (_desc.device.iProduct != 0) {
            xfer_result = tuh_descriptor_get_product_string_sync(
                daddr, LANGUAGE_ID, _desc.buf, sizeof(_desc.buf));
            if (XFER_RESULT_SUCCESS == xfer_result) {
                _parse_string_descriptor(_desc.buf, sizeof(_desc.buf) / 2,
                                         dev->product, sizeof(dev->product));
            }
        }
        if (dev->product[0] == '\0') {
            snprintf(dev->product, sizeof(dev->product), "USB Device");
        }

        /* Serial number string */
        if (_desc.device.iSerialNumber != 0) {
            xfer_result = tuh_descriptor_get_serial_string_sync(
                daddr, LANGUAGE_ID, _desc.buf, sizeof(_desc.buf));
            if (XFER_RESULT_SUCCESS == xfer_result) {
                _parse_string_descriptor(_desc.buf, sizeof(_desc.buf) / 2,
                                         dev->serial, sizeof(dev->serial));
            }
        }
        if (dev->serial[0] == '\0') {
            snprintf(dev->serial, sizeof(dev->serial), "N/A");
        }

        dev->strings_ready = true;

        printf("[USB] Manufacturer: %s\n", dev->manufacturer);
        printf("[USB] Product:      %s\n", dev->product);
        printf("[USB] Serial:       %s\n", dev->serial);
    }

    /* Notify threat analyzer */
    threat_add_device(dev);

    printf("[USB] Device %d fully enumerated\n", daddr);
}

/**
 * @brief Called by TinyUSB when a device is unmounted (disconnected).
 */
void tuh_umount_cb(uint8_t daddr) {
    printf("[USB] Device unmounted at address %d\n", daddr);

    usb_device_info_t *dev = _find_device(daddr);
    if (dev) {
        /* Check if this was a hub */
        if (dev->usb_class == 0x09) {
            g_hub_connected = false;
            printf("[USB] Hub disconnected\n");
        }

        /* Notify threat analyzer and HID monitor */
        threat_remove_device(daddr);
        if (dev->is_hid) {
            hid_monitor_remove_device(daddr);
        }

        /* Clear the slot */
        memset(dev, 0, sizeof(*dev));
        printf("[USB] Device %d removed\n", daddr);
    } else {
        printf("[USB] WARNING: Unmount for unknown device %d\n", daddr);
    }
}

/* ============================================================================
 * TinyUSB HID CALLBACKS
 * ============================================================================ */

/**
 * @brief Called when a HID interface is mounted on a device.
 *
 * We mark the device as HID, register it with hid_monitor, and start
 * receiving reports via tuh_hid_receive_report().
 */
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                       uint8_t const *desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;

    printf("[HID] HID mounted: dev_addr=%d instance=%d\n", dev_addr, instance);

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    const char *protocol_str[] = {"None", "Keyboard", "Mouse"};
    printf("[HID] Interface Protocol = %s\n",
           (itf_protocol < 3) ? protocol_str[itf_protocol] : "Unknown");

    /* Mark the device as HID */
    usb_device_info_t *dev = _find_device(dev_addr);
    if (dev) {
        dev->is_hid = true;
        dev->instance = instance;

        /* If device class was 0 (defined at interface level), set it to HID */
        if (dev->usb_class == 0) {
            dev->usb_class = 0x03;  /* HID class */
        }

        /* Re-notify threat analyzer with updated device info (is_hid is now true)
         * so it re-classifies from SAFE to POTENTIALLY_UNSAFE */
        threat_update_device_info(dev);
    }

    /* Register with HID monitor for keystroke rate tracking */
    hid_monitor_add_device(dev_addr);

    /* Start receiving HID reports */
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        printf("[HID] ERROR: Cannot request report from dev_addr=%d instance=%d\n",
               dev_addr, instance);
    }
}

/**
 * @brief Called when a HID interface is unmounted.
 */
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    printf("[HID] HID unmounted: dev_addr=%d instance=%d\n", dev_addr, instance);
}

/**
 * @brief Called when a HID report is received from a device.
 *
 * Feeds the report to hid_monitor (keystroke rate tracking) and
 * threat_analyzer (attack detection), then re-requests the next report.
 */
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                 uint8_t const *report, uint16_t len) {
    /* Feed to HID monitor for keystroke rate analysis */
    hid_monitor_report(dev_addr, report, len);

    /* Feed to threat analyzer for attack detection */
    threat_update_hid_activity(dev_addr, len);

    /* Continue requesting reports */
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        printf("[HID] ERROR: Cannot re-request report from dev_addr=%d instance=%d\n",
               dev_addr, instance);
    }
}
