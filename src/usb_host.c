/*
 * PlugSafe USB Host Layer Implementation (Stub for now)
 * Device enumeration, descriptor parsing, and HID monitoring
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "usb_host.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"

/* Device tracking */
static usb_device_info_t g_usb_devices[4];  /* Max 4 devices */
static uint8_t g_device_count = 0;

/* Get current time in milliseconds */
static uint64_t get_time_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

/* ===== Public API ===== */

bool usb_host_init(void) {
    printf("[USB] Initializing USB host stack...\n");
    
    /* Initialize device array */
    memset(g_usb_devices, 0, sizeof(g_usb_devices));
    g_device_count = 0;
    
    /* Note: Full TinyUSB implementation requires pio_usb library
     * For now, this is a stub that allows the application to run
     * and display the OLED interface. USB device detection can be
     * added later when full TinyUSB is available. */
    
    printf("[USB] USB host initialized (stub mode - device detection not available yet)\n");
    printf("[USB] TODO: Implement full TinyUSB with PIO USB support\n");
    
    return true;
}

void usb_host_task(void) {
    /* USB polling would happen here */
    /* For now, this is a no-op stub */
}

usb_device_info_t* usb_get_device_info(uint8_t dev_addr) {
    for (int i = 0; i < 4; i++) {
        if (g_usb_devices[i].dev_addr == dev_addr && g_usb_devices[i].is_mounted) {
            return &g_usb_devices[i];
        }
    }
    return NULL;
}

uint8_t usb_get_device_count(void) {
    uint8_t count = 0;
    for (int i = 0; i < 4; i++) {
        if (g_usb_devices[i].is_mounted) {
            count++;
        }
    }
    return count;
}

usb_device_info_t* usb_get_device_at_index(uint8_t index) {
    uint8_t count = 0;
    for (int i = 0; i < 4; i++) {
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
    usb_device_info_t *dev = usb_get_device_info(dev_addr);
    return (dev != NULL);
}

/* ===== TinyUSB Callbacks ===== */
/* These would be called by TinyUSB in the full implementation */

void tuh_mount_cb(uint8_t dev_addr) {
    /* Stub - would be called when USB device mounts */
    printf("[USB] Device mounted callback (stub)\n");
}

void tuh_umount_cb(uint8_t dev_addr) {
    /* Stub - would be called when USB device unmounts */
    printf("[USB] Device unmounted callback (stub)\n");
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    /* Stub - would be called when HID report is received */
}
