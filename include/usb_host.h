/*
 * PlugSafe USB Host Layer
 * Device enumeration, descriptor parsing, and HID monitoring
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef USB_HOST_H
#define USB_HOST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* USB Device Information Structure */
typedef struct {
    uint8_t dev_addr;           /* TinyUSB device address */
    uint16_t vid;               /* Vendor ID */
    uint16_t pid;               /* Product ID */
    uint8_t usb_class;          /* USB Device Class */
    uint8_t subclass;           /* USB Device Subclass */
    uint8_t protocol;           /* USB Device Protocol */
    char manufacturer[64];      /* Manufacturer string */
    char product[64];           /* Product string */
    char serial[64];            /* Serial number string */
    bool is_hid;                /* Is this a HID device? */
    bool is_mounted;            /* Currently mounted? */
    uint64_t connected_time_ms; /* Time device was connected */
} usb_device_info_t;

/* USB Host Initialization */
bool usb_host_init(void);

/* Must be called regularly in main loop */
void usb_host_task(void);

/* Query device information */
usb_device_info_t* usb_get_device_info(uint8_t dev_addr);

/* Get count of connected devices */
uint8_t usb_get_device_count(void);

/* Get device at index (0 to count-1) */
usb_device_info_t* usb_get_device_at_index(uint8_t index);

/* Check if specific device address is mounted */
bool usb_is_device_mounted(uint8_t dev_addr);

/* Check if a USB hub is currently connected (warning state) */
bool usb_is_hub_connected(void);

/* GPIO-based device registration (for passive USB detection) */
/**
 * @brief Register a device detected by GPIO (when D+ or D- goes high)
 * @param vid Vendor ID (use 0x0000 if unknown)
 * @param pid Product ID (use 0x0000 if unknown)
 * @return true if registered, false if device array is full
 */
bool usb_register_gpio_device(uint16_t vid, uint16_t pid);

/**
 * @brief Unregister device detected by GPIO (when D+ and D- go low)
 * Removes the most recently added device from tracking
 */
void usb_unregister_gpio_device(void);

#endif /* USB_HOST_H */
