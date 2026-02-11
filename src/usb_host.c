/*
 * PlugSafe USB Host Layer Implementation
 * Device enumeration, descriptor parsing, and HID monitoring using TinyUSB
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

/* ============================================================================
 * DEVICE TRACKING & STATE MANAGEMENT
 * ============================================================================ */

/* Connected USB devices */
static usb_device_info_t g_usb_devices[4];
static uint8_t g_device_count = 0;

/* Track if a USB hub is connected (warning flag) */
static bool g_hub_connected = false;

/* ============================================================================
 * HELPER FUNCTIONS - DEVICE CLASSIFICATION & PARSING
 * ============================================================================ */

/**
 * @brief Check if a device is a HID class device
 * @param class_code USB device class code
 * @return true if class is HID (0x03)
 */
static bool is_hid_device(uint8_t class_code) {
    return (class_code == 0x03);
}

/**
 * @brief Check if a device is a USB Hub
 * @param class_code USB device class code
 * @return true if class is Hub (0x09)
 */
static bool is_usb_hub(uint8_t class_code) {
    return (class_code == 0x09);
}

/* ============================================================================
 * PUBLIC API FUNCTIONS
 * ============================================================================ */

bool usb_host_init(void) {
    printf("[USB] Initializing USB host stack...\n");
    printf("[USB] [INFO] USB Host mode is disabled (TinyUSB not available)\n");
    printf("[USB] [INFO] GPIO-based passive USB detection is active instead\n");
    printf("[USB] [INFO] GPIO 0 (D+) and GPIO 1 (D-) in use for passive detection\n");
    
    /* Initialize device array */
    memset(g_usb_devices, 0, sizeof(g_usb_devices));
    g_device_count = 0;
    g_hub_connected = false;
    
    return true;
}

void usb_host_task(void) {
    /* TinyUSB host mode is disabled */
    /* GPIO-based detection handled in usb_detector.c */
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

/**
 * @brief Check if a USB hub is currently connected
 * @return true if hub is plugged in
 */
bool usb_is_hub_connected(void) {
    return g_hub_connected;
}

/**
 * @brief Register a device detected by GPIO (for passive detection)
 * Called when GPIO detects USB device connection
 * @param vid Vendor ID (0x0000 for generic/unknown)
 * @param pid Product ID (0x0000 for generic/unknown)
 * @return true if device was registered, false if array is full
 */
bool usb_register_gpio_device(uint16_t vid, uint16_t pid) {
    /* Find free slot */
    for (int i = 0; i < 4; i++) {
        if (!g_usb_devices[i].is_mounted) {
            usb_device_info_t *dev = &g_usb_devices[i];
            memset(dev, 0, sizeof(*dev));
            
            /* Set device information */
            dev->dev_addr = i + 1;  /* Address 1-4 for GPIO-detected devices */
            dev->vid = vid;
            dev->pid = pid;
            dev->usb_class = 0xFF;  /* Unknown class for GPIO detection */
            dev->subclass = 0x00;
            dev->protocol = 0x00;
            dev->is_hid = false;
            dev->is_mounted = true;
            dev->connected_time_ms = to_ms_since_boot(get_absolute_time());
            
            /* Set default strings */
            snprintf(dev->manufacturer, sizeof(dev->manufacturer), "Unknown");
            snprintf(dev->product, sizeof(dev->product), "USB Device");
            snprintf(dev->serial, sizeof(dev->serial), "N/A");
            
            g_device_count++;
            
            printf("[USB] GPIO Device registered at address %d\n", dev->dev_addr);
            printf("[USB] [INFO] Device connected via GPIO detection\n");
            
            /* Notify threat analyzer */
            extern void threat_add_device(const usb_device_info_t *dev_info);
            threat_add_device(dev);
            
            return true;
        }
    }
    
    printf("[USB] [ERROR] Cannot register device - array full\n");
    return false;
}

/**
 * @brief Unregister device detected by GPIO (for passive detection)
 * Called when GPIO detects USB device disconnection
 */
void usb_unregister_gpio_device(void) {
    /* Remove first mounted device (simple approach for single port) */
    for (int i = 0; i < 4; i++) {
        if (g_usb_devices[i].is_mounted) {
            printf("[USB] GPIO Device unregistered from address %d\n", g_usb_devices[i].dev_addr);
            printf("[USB] [INFO] Device disconnected via GPIO detection\n");
            
            /* Notify threat analyzer */
            extern void threat_remove_device(uint8_t dev_addr);
            threat_remove_device(g_usb_devices[i].dev_addr);
            
            /* Clear entry */
            memset(&g_usb_devices[i], 0, sizeof(g_usb_devices[i]));
            g_device_count--;
            
            return;
        }
    }
}

/* ============================================================================
 * TINYUSB CALLBACK STUBS
 * ============================================================================
 * 
 * These callbacks would be called by TinyUSB when USB host mode is enabled.
 * Currently stubbed out since we use GPIO-based detection instead.
 */

void tuh_mount_cb(uint8_t dev_addr) {
    (void)dev_addr;
}

void tuh_umount_cb(uint8_t dev_addr) {
    (void)dev_addr;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, 
                                 uint8_t const *report, uint16_t len) {
    (void)dev_addr;
    (void)instance;
    (void)report;
    (void)len;
}

void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t idx, 
                                      uint8_t report_id, uint8_t report_type, 
                                      uint16_t len) {
    (void)dev_addr;
    (void)idx;
    (void)report_id;
    (void)report_type;
    (void)len;
}

void tuh_descriptor_string_get_sync_cb(uint8_t dev_addr, uint8_t request_idx, 
                                        uint16_t language_id) {
    (void)dev_addr;
    (void)request_idx;
    (void)language_id;
}

/* ============================================================================
 * THREAT ANALYZER INTEGRATION STUBS
 * ============================================================================ */

/**
 * @brief Add device to threat analyzer
 * Defined here as external; actual implementation in threat_analyzer.c
 */
__attribute__((weak))
void threat_add_device(const usb_device_info_t *dev_info) {
    (void)dev_info;
    /* Weak implementation - will be overridden by threat_analyzer.c */
}

/**
 * @brief Remove device from threat analyzer
 * Defined here as external; actual implementation in threat_analyzer.c
 */
__attribute__((weak))
void threat_remove_device(uint8_t dev_addr) {
    (void)dev_addr;
    /* Weak implementation - will be overridden by threat_analyzer.c */
}

/**
 * @brief Update HID activity in threat analyzer
 * Defined here as external; actual implementation in threat_analyzer.c
 */
__attribute__((weak))
void threat_update_hid_activity(uint8_t dev_addr, uint16_t report_len) {
    (void)dev_addr;
    (void)report_len;
    /* Weak implementation - will be overridden by threat_analyzer.c */
}
