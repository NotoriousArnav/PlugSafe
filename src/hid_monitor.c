/*
 * PlugSafe HID Monitor Implementation
 * Keystroke rate detection and analysis
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "hid_monitor.h"
#include <stdio.h>
#include <string.h>
#include "pico/time.h"

/* HID Monitor tracking */
static hid_monitor_t g_hid_monitors[MAX_HID_DEVICES];

/* Helper: Get current time in ms */
static uint64_t get_time_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

/* ===== Public API ===== */

void hid_monitor_init(void) {
    printf("[HID] Initializing HID keystroke rate monitor...\n");
    memset(g_hid_monitors, 0, sizeof(g_hid_monitors));
    printf("[HID] Keystroke threshold: %d keys/sec (malicious if exceeded)\n", HID_KEYSTROKE_THRESHOLD_HZ);
    printf("[HID] Measurement window: %d ms\n", KEYSTROKE_RATE_WINDOW_MS);
}

void hid_monitor_add_device(uint8_t dev_addr) {
    /* Find free slot */
    for (int i = 0; i < MAX_HID_DEVICES; i++) {
        if (!g_hid_monitors[i].is_monitoring) {
            hid_monitor_t *mon = &g_hid_monitors[i];
            memset(mon, 0, sizeof(*mon));
            mon->dev_addr = dev_addr;
            mon->is_monitoring = true;
            mon->last_window_start_ms = get_time_ms();
            printf("[HID] Started monitoring HID device at address: %d\n", dev_addr);
            return;
        }
    }
    printf("[HID] WARNING: No free HID monitor slots for device %d\n", dev_addr);
}

void hid_monitor_report(uint8_t dev_addr, const uint8_t *report, uint16_t len) {
    hid_monitor_t *mon = hid_get_monitor_stats(dev_addr);
    
    if (!mon) {
        return;
    }
    
    uint64_t now = get_time_ms();
    mon->total_reports++;
    mon->reports_this_second++;
    
    /* Check if we need to update the window */
    if ((now - mon->last_window_start_ms) >= KEYSTROKE_RATE_WINDOW_MS) {
        /* Calculate rate for completed window */
        uint64_t elapsed_ms = now - mon->last_window_start_ms;
        mon->current_rate_hz = (mon->reports_this_second * 1000) / elapsed_ms;
        
        /* Track peak rate */
        if (mon->current_rate_hz > mon->peak_rate_hz) {
            mon->peak_rate_hz = mon->current_rate_hz;
        }
        
        /* Reset window */
        mon->last_window_start_ms = now;
        mon->reports_this_second = 0;
        
        /* Log if spammy */
        if (mon->current_rate_hz > HID_KEYSTROKE_THRESHOLD_HZ) {
            printf("[HID] 🚨 ALERT: Keystroke rate %u keys/sec (threshold: %d) from device %d\n",
                   mon->current_rate_hz, HID_KEYSTROKE_THRESHOLD_HZ, dev_addr);
        }
    }
}

uint32_t hid_get_keystroke_rate(uint8_t dev_addr) {
    hid_monitor_t *mon = hid_get_monitor_stats(dev_addr);
    return (mon) ? mon->current_rate_hz : 0;
}

bool hid_is_spammy(uint8_t dev_addr) {
    hid_monitor_t *mon = hid_get_monitor_stats(dev_addr);
    return (mon && mon->current_rate_hz > HID_KEYSTROKE_THRESHOLD_HZ);
}

void hid_monitor_remove_device(uint8_t dev_addr) {
    for (int i = 0; i < MAX_HID_DEVICES; i++) {
        if (g_hid_monitors[i].dev_addr == dev_addr && g_hid_monitors[i].is_monitoring) {
            printf("[HID] Stopped monitoring HID device at address: %d (peak rate: %u keys/sec)\n",
                   dev_addr, g_hid_monitors[i].peak_rate_hz);
            memset(&g_hid_monitors[i], 0, sizeof(g_hid_monitors[i]));
            return;
        }
    }
}

hid_monitor_t* hid_get_monitor_stats(uint8_t dev_addr) {
    for (int i = 0; i < MAX_HID_DEVICES; i++) {
        if (g_hid_monitors[i].dev_addr == dev_addr && g_hid_monitors[i].is_monitoring) {
            return &g_hid_monitors[i];
        }
    }
    return NULL;
}
