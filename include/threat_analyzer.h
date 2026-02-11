/*
 * PlugSafe Threat Analyzer
 * Device classification: Safe, Potentially Unsafe, Malicious
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef THREAT_ANALYZER_H
#define THREAT_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>
#include "usb_host.h"

/* Threat Level Classification */
typedef enum {
    THREAT_SAFE = 0,                  /* Non-HID device - automatically safe */
    THREAT_POTENTIALLY_UNSAFE = 1,    /* HID device - requires monitoring */
    THREAT_MALICIOUS = 2              /* HID device with attack signature */
} threat_level_e;

/* Complete Device Threat Status */
typedef struct {
    uint8_t dev_addr;
    usb_device_info_t device;
    threat_level_e threat_level;
    uint32_t hid_report_count;         /* Total HID reports received */
    uint32_t hid_reports_per_sec;      /* Current keystroke rate (keys/sec) */
    bool is_active;
} device_threat_t;

/* Configuration Constants */
#define HID_KEYSTROKE_THRESHOLD_HZ    50    /* Flag malicious if > 50 keys/sec */
#define RATE_NORMAL_MAX_HZ            30    /* Normal human typing max */
#define RATE_SUSPICIOUS_MIN_HZ        50    /* Start suspicion here */

/* Initialize threat analyzer */
void threat_analyzer_init(void);

/* Analyze device descriptor and return threat level */
threat_level_e threat_analyze_device(const usb_device_info_t *info);

/* Update HID activity for a device */
void threat_update_hid_activity(uint8_t dev_addr, uint16_t report_len);

/* Get current threat level for a device */
threat_level_e threat_get_current_level(uint8_t dev_addr);

/* Get threat status for a device */
device_threat_t* threat_get_device_status(uint8_t dev_addr);

/* Get threat status by index */
device_threat_t* threat_get_device_at_index(uint8_t index);

/* Check if keystroke rate is spammy (attack signature) */
bool threat_is_hid_spammy(uint8_t dev_addr);

/* Remove device from threat tracking */
void threat_remove_device(uint8_t dev_addr);

/* Add device to threat tracking (called by USB host) */
void threat_add_device(const usb_device_info_t *dev_info);

#endif /* THREAT_ANALYZER_H */
