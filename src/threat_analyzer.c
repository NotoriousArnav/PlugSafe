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
#include "hid_monitor.h"
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
        /* Mice (protocol 2) are safe — high report rates are normal mouse movement */
        if (info->hid_protocol == 2) {
            printf("[THREAT] Device '%s' is HID Mouse - Classification: SAFE ✅\n",
                   info->product[0] ? info->product : "Unknown");
            printf("[THREAT] Reason: Mouse input, no keystroke injection risk\n");
            return THREAT_SAFE;
        }
        
        /* Keyboards (protocol 1) and unknown HID (protocol 0) need monitoring */
        const char *type_str = (info->hid_protocol == 1) ? "Keyboard" : "HID";
        printf("[THREAT] Device '%s' is %s - Classification: POTENTIALLY_UNSAFE ⚠️\n",
               info->product[0] ? info->product : "Unknown", type_str);
        printf("[THREAT] Reason: %s devices require keystroke rate monitoring\n", type_str);
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
        
        /* Use the windowed keystroke rate from hid_monitor (1-second sliding window)
         * instead of computing an all-time average which dilutes burst detection */
        uint32_t windowed_rate = hid_get_keystroke_rate(dev_addr);
        threat->hid_reports_per_sec = windowed_rate;
        
        /* Ensure the device is marked as HID in our snapshot (it may have been
         * added before tuh_hid_mount_cb fired) */
        if (!threat->device.is_hid) {
            usb_device_info_t *live_dev = usb_get_device_info(dev_addr);
            if (live_dev && live_dev->is_hid) {
                threat->device.is_hid = true;
                if (threat->threat_level < THREAT_POTENTIALLY_UNSAFE) {
                    threat->threat_level = THREAT_POTENTIALLY_UNSAFE;
                }
            }
        }
        
        /* Check if spammy (malicious) — MALICIOUS is sticky, never de-escalates */
        if (windowed_rate > HID_KEYSTROKE_THRESHOLD_HZ) {
            if (threat->threat_level != THREAT_MALICIOUS) {
                printf("\n[THREAT] 🚨 THREAT ESCALATION 🚨\n");
                printf("[THREAT] Device '%s' detected with rapid keystroke rate!\n",
                       threat->device.product[0] ? threat->device.product : "Unknown");
                printf("[THREAT] Rate: %u keys/sec (threshold: %d keys/sec)\n",
                       windowed_rate, HID_KEYSTROKE_THRESHOLD_HZ);
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
    
    /* Lookup only — do NOT auto-create. Device must be added via threat_add_device(). */
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

void threat_update_device_info(const usb_device_info_t *dev_info) {
    if (!dev_info) {
        return;
    }
    
    /* Find existing tracked device by dev_addr */
    for (int i = 0; i < MAX_TRACKED_DEVICES; i++) {
        if (g_threat_devices[i].device.dev_addr == dev_info->dev_addr &&
            g_threat_devices[i].device.is_mounted) {
            device_threat_t *threat = &g_threat_devices[i];
            
            /* Update the device snapshot with latest info */
            memcpy(&threat->device, dev_info, sizeof(*dev_info));
            
            /* Re-classify threat level (only escalate, never de-escalate) */
            threat_level_e new_level = threat_analyze_device(dev_info);
            if (new_level > threat->threat_level) {
                threat->threat_level = new_level;
                printf("[THREAT] Device '%s' re-classified to level %d\n",
                       dev_info->product[0] ? dev_info->product : "Unknown",
                       new_level);
            }
            
            return;
        }
    }
    
    /* Device not found in threat tracker — add it */
    printf("[THREAT] Device %d not tracked yet, adding via update\n", dev_info->dev_addr);
    threat_add_device(dev_info);
}

