/*
 * PlugSafe Threat Analyzer Implementation
 * Device classification: Safe, Potentially Unsafe, Malicious
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "threat_analyzer.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb_config.h"

/* Threat tracking for connected devices (max 4 devices) */
#define MAX_TRACKED_DEVICES 4
static device_threat_t g_threat_devices[MAX_TRACKED_DEVICES];

/* ===== Public API ===== */

void threat_analyzer_init(void) {
    printf("[THREAT] Initializing threat analyzer...\n");
    memset(g_threat_devices, 0, sizeof(g_threat_devices));
    printf("[THREAT] Threat analyzer ready\n");
    printf("[THREAT] Classification:\n");
    printf("  - SAFE: Non-HID devices (USB drives, audio devices, etc.)\n");
    printf("  - POTENTIALLY_UNSAFE: HID devices (keyboard/mouse - monitor keystroke rate)\n");
    printf("  - MALICIOUS: HID with keystroke rate > %d keys/sec\n", HID_KEYSTROKE_THRESHOLD_HZ);
}

threat_level_e threat_analyze_device(const usb_device_info_t *info) {
    if (!info) {
        return THREAT_SAFE;
    }
    
    /* Check if HID device */
    if (info->is_hid) {
        printf("[THREAT] Device '%s' is HID class - Classification: POTENTIALLY_UNSAFE ⚠️\n",
               info->product[0] ? info->product : "Unknown");
        printf("[THREAT] Reason: HID devices require keystroke rate monitoring\n");
        return THREAT_POTENTIALLY_UNSAFE;
    }
    
    /* Non-HID device */
    printf("[THREAT] Device '%s' is non-HID (class 0x%02X) - Classification: SAFE ✅\n",
           info->product[0] ? info->product : "Unknown", info->usb_class);
    printf("[THREAT] Reason: Not a keyboard/mouse input device\n");
    return THREAT_SAFE;
}

void threat_update_hid_activity(uint8_t dev_addr, uint16_t report_len) {
    device_threat_t *threat = threat_get_device_status(dev_addr);
    
    if (threat) {
        threat->hid_report_count++;
        
        /* Calculate keystroke rate (very simple: assume 1 keystroke per report) */
        uint64_t time_ms = to_ms_since_boot(get_absolute_time());
        uint64_t elapsed_ms = time_ms - threat->device.connected_time_ms;
        threat->hid_reports_per_sec = (elapsed_ms > 0) ? (threat->hid_report_count * 1000) / elapsed_ms : 0;
        
        /* Check if spammy (malicious) */
        if (threat->hid_reports_per_sec > HID_KEYSTROKE_THRESHOLD_HZ) {
            if (threat->threat_level != THREAT_MALICIOUS) {
                printf("\n[THREAT] 🚨 THREAT ESCALATION 🚨\n");
                printf("[THREAT] Device '%s' detected with rapid keystroke rate!\n",
                       threat->device.product[0] ? threat->device.product : "Unknown");
                printf("[THREAT] Rate: %u keys/sec (threshold: %d keys/sec)\n",
                       threat->hid_reports_per_sec, HID_KEYSTROKE_THRESHOLD_HZ);
                printf("[THREAT] Classification: MALICIOUS 🚨\n");
                printf("[THREAT] RECOMMENDATION: DISCONNECT DEVICE IMMEDIATELY\n");
                printf("[THREAT] This appears to be an automated keystroke injection attack\n");
                printf("[THREAT] (e.g., Rubber Ducky, BadUSB, or similar malware)\n\n");
            }
            threat->threat_level = THREAT_MALICIOUS;
        }
    }
}

threat_level_e threat_get_current_level(uint8_t dev_addr) {
    device_threat_t *threat = threat_get_device_status(dev_addr);
    return (threat) ? threat->threat_level : THREAT_SAFE;
}

device_threat_t* threat_get_device_status(uint8_t dev_addr) {
    for (int i = 0; i < MAX_TRACKED_DEVICES; i++) {
        if (g_threat_devices[i].device.dev_addr == dev_addr && 
            g_threat_devices[i].device.is_mounted) {
            return &g_threat_devices[i];
        }
    }
    
    /* Not found - create new entry */
    for (int i = 0; i < MAX_TRACKED_DEVICES; i++) {
        if (!g_threat_devices[i].device.is_mounted) {
            device_threat_t *threat = &g_threat_devices[i];
            memset(threat, 0, sizeof(*threat));
            threat->device.dev_addr = dev_addr;
            
            /* Analyze device (assumption: must call usb_host_init first to populate device info) */
            usb_device_info_t *usb_info = usb_get_device_info(dev_addr);
            if (usb_info) {
                memcpy(&threat->device, usb_info, sizeof(*usb_info));
                threat->threat_level = threat_analyze_device(usb_info);
            }
            
            threat->is_active = true;
            return threat;
        }
    }
    
    return NULL;
}

device_threat_t* threat_get_device_at_index(uint8_t index) {
    uint8_t count = 0;
    for (int i = 0; i < MAX_TRACKED_DEVICES; i++) {
        if (g_threat_devices[i].device.is_mounted) {
            if (count == index) {
                return &g_threat_devices[i];
            }
            count++;
        }
    }
    return NULL;
}

bool threat_is_hid_spammy(uint8_t dev_addr) {
    device_threat_t *threat = threat_get_device_status(dev_addr);
    return (threat && threat->threat_level == THREAT_MALICIOUS);
}

void threat_remove_device(uint8_t dev_addr) {
    for (int i = 0; i < MAX_TRACKED_DEVICES; i++) {
        if (g_threat_devices[i].device.dev_addr == dev_addr) {
            printf("[THREAT] Device '%s' removed from threat tracking\n",
                   g_threat_devices[i].device.product[0] ? g_threat_devices[i].device.product : "Unknown");
            memset(&g_threat_devices[i], 0, sizeof(g_threat_devices[i]));
            return;
        }
    }
}

void threat_add_device(const usb_device_info_t *dev_info) {
    if (!dev_info) {
        return;
    }
    
    /* Find free slot */
    for (int i = 0; i < MAX_TRACKED_DEVICES; i++) {
        if (!g_threat_devices[i].device.is_mounted) {
            device_threat_t *threat = &g_threat_devices[i];
            memset(threat, 0, sizeof(*threat));
            
            /* Copy device info */
            memcpy(&threat->device, dev_info, sizeof(*dev_info));
            
            /* Analyze threat level */
            threat->threat_level = threat_analyze_device(dev_info);
            threat->is_active = true;
            
            printf("[THREAT] Device '%s' added to threat tracking\n",
                   dev_info->product[0] ? dev_info->product : "Unknown");
            
            return;
        }
    }
    
    printf("[THREAT] [ERROR] Threat tracking array full, cannot add device\n");
}

