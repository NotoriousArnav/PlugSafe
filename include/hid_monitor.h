/*
 * PlugSafe HID Monitor
 * Keystroke rate detection and analysis
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef HID_MONITOR_H
#define HID_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

/* Configuration */
#define HID_KEYSTROKE_THRESHOLD_HZ    50    /* Flag malicious if > 50 keys/sec */
#define KEYSTROKE_RATE_WINDOW_MS      1000  /* Measure over 1 second window */
#define MAX_HID_DEVICES               4     /* Max simultaneous HID devices */

/* HID Monitor Statistics */
typedef struct {
    uint8_t dev_addr;
    uint32_t total_reports;           /* Total HID reports received */
    uint32_t reports_this_second;     /* Reports in current 1-second window */
    uint64_t last_window_start_ms;    /* Start time of current window */
    uint32_t peak_rate_hz;            /* Peak keystroke rate seen */
    uint32_t current_rate_hz;         /* Current keystroke rate */
    bool is_monitoring;               /* Currently monitoring this device */
} hid_monitor_t;

/* Initialize HID monitor */
void hid_monitor_init(void);

/* Register a HID device for monitoring */
void hid_monitor_add_device(uint8_t dev_addr);

/* Process HID report and update keystroke rate */
void hid_monitor_report(uint8_t dev_addr, const uint8_t *report, uint16_t len);

/* Get keystroke rate for a device (keys/sec) */
uint32_t hid_get_keystroke_rate(uint8_t dev_addr);

/* Check if keystroke rate is spammy/malicious */
bool hid_is_spammy(uint8_t dev_addr);

/* Remove device from monitoring */
void hid_monitor_remove_device(uint8_t dev_addr);

/* Get monitor stats for a device */
hid_monitor_t* hid_get_monitor_stats(uint8_t dev_addr);

#endif /* HID_MONITOR_H */
