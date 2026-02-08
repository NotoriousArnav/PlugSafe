/*
 * PlugSafe - Malicious USB Detector with OLED Display
 * USB Host + OLED Display Integration
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "pico/time.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "oled_i2c.h"
#include "oled_driver.h"
#include "oled_display.h"
#include "oled_graphics.h"
#include "oled_text.h"
#include "oled_font.h"
#include "usb_host.h"
#include "threat_analyzer.h"
#include "hid_monitor.h"

/* GPIO pins for LED */
#define LED_PIN 25

/* I2C pins for OLED display */
#define I2C_SDA_PIN 20  /* GPIO 20 for I2C SDA (data line) */
#define I2C_SCL_PIN 21  /* GPIO 21 for I2C SCL (clock line) */
#define I2C_PORT i2c0
#define I2C_BAUDRATE 400000

/* OLED display address */
#define OLED_ADDRESS 0x3C

/* BOOTSEL button - native hardware button */
#define BOOTSEL_PIN 24  /* BOOTSEL is GPIO 24 on Pico */

/* Display update timing (in milliseconds) */
#define DISPLAY_UPDATE_INTERVAL_MS 500
#define USB_HOST_POLL_INTERVAL_MS  10

/* Current display page (0-2 for normal pages) */
static uint8_t current_page = 0;

/* Time tracking for display updates */
static uint64_t last_display_update_ms = 0;
static uint64_t last_usb_poll_ms = 0;

/* Helper: Get current time in milliseconds */
static uint64_t get_time_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

/* Draw page 1: Status Overview */
static void draw_page_status(oled_display_t *display, const oled_font_t *font) {
    oled_display_clear(display);
    
    /* Title */
    oled_draw_string(display, 10, 0, "PlugSafe Status", font, true);
    oled_draw_hline(display, 0, 10, 128, true);
    
    /* Device count */
    uint8_t device_count = usb_get_device_count();
    char status_str[32];
    snprintf(status_str, sizeof(status_str), "Devices: %d/4", device_count);
    oled_draw_string(display, 5, 15, status_str, font, true);
    
    /* Overall threat status */
    threat_level_e max_threat = THREAT_SAFE;
    for (uint8_t i = 0; i < device_count; i++) {
        device_threat_t *threat = threat_get_device_at_index(i);
        if (threat && threat->threat_level > max_threat) {
            max_threat = threat->threat_level;
        }
    }
    
    const char *status_text;
    switch (max_threat) {
        case THREAT_MALICIOUS:
            status_text = "THREAT DETECTED!";
            break;
        case THREAT_POTENTIALLY_UNSAFE:
            status_text = "MONITORING HID";
            break;
        default:
            status_text = "All Clear";
    }
    oled_draw_string(display, 5, 25, status_text, font, true);
    
    /* Device listing */
    if (device_count > 0) {
        oled_draw_string(display, 5, 40, "Devices:", font, true);
        for (uint8_t i = 0; i < device_count && i < 2; i++) {
            usb_device_info_t *dev = usb_get_device_at_index(i);
            if (dev) {
                const char *type = dev->is_hid ? "HID" : "USB";
                snprintf(status_str, sizeof(status_str), "%s:%04X/%04X", type, dev->vid, dev->pid);
                oled_draw_string(display, 10, 50 + (i * 8), status_str, font, false);
            }
        }
    } else {
        oled_draw_string(display, 5, 50, "No USB devices", font, true);
    }
    
    /* Page indicator */
    oled_draw_string(display, 110, 55, "1/3", font, false);
}

/* Draw page 2: Device Details */
static void draw_page_details(oled_display_t *display, const oled_font_t *font) {
    oled_display_clear(display);
    
    /* Title */
    oled_draw_string(display, 10, 0, "Device Details", font, true);
    oled_draw_hline(display, 0, 10, 128, true);
    
    /* Show first device details */
    usb_device_info_t *dev = usb_get_device_at_index(0);
    
    if (dev) {
        char info_str[32];
        snprintf(info_str, sizeof(info_str), "VID:%04X PID:%04X", dev->vid, dev->pid);
        oled_draw_string(display, 5, 15, info_str, font, false);
        
        oled_draw_string(display, 5, 25, "Product:", font, true);
        oled_draw_string(display, 5, 33, dev->product[0] ? dev->product : "Unknown", font, false);
        
        if (dev->manufacturer[0]) {
            oled_draw_string(display, 5, 45, "Mfg: ", font, true);
            oled_draw_string(display, 35, 45, dev->manufacturer, font, false);
        }
    } else {
        oled_draw_string(display, 5, 30, "No devices", font, true);
    }
    
    /* Page indicator */
    oled_draw_string(display, 110, 55, "2/3", font, false);
}

/* Draw page 3: HID Monitor */
static void draw_page_hid_monitor(oled_display_t *display, const oled_font_t *font) {
    oled_display_clear(display);
    
    /* Title */
    oled_draw_string(display, 10, 0, "HID Monitor", font, true);
    oled_draw_hline(display, 0, 10, 128, true);
    
    /* Show HID activity */
    device_threat_t *threat = threat_get_device_at_index(0);
    
    if (threat && threat->device.is_hid) {
        char info_str[32];
        
        snprintf(info_str, sizeof(info_str), "Rate: %u k/s", threat->hid_reports_per_sec);
        oled_draw_string(display, 5, 15, info_str, font, true);
        
        snprintf(info_str, sizeof(info_str), "Count: %u", threat->hid_report_count);
        oled_draw_string(display, 5, 25, info_str, font, true);
        
        const char *threat_text;
        switch (threat->threat_level) {
            case THREAT_MALICIOUS:
                threat_text = "MALICIOUS!";
                break;
            case THREAT_POTENTIALLY_UNSAFE:
                threat_text = "MONITORING";
                break;
            default:
                threat_text = "SAFE";
        }
        oled_draw_string(display, 5, 40, threat_text, font, true);
        
        /* Threat level bar */
        if (threat->hid_reports_per_sec > HID_KEYSTROKE_THRESHOLD_HZ) {
            oled_draw_rect(display, 5, 50, 118, 10, true, true);
        } else if (threat->device.is_hid) {
            oled_draw_rect(display, 5, 50, 118, 10, false, true);
        }
    } else {
        oled_draw_string(display, 5, 30, "No HID devices", font, true);
    }
    
    /* Page indicator */
    oled_draw_string(display, 110, 55, "3/3", font, false);
}

/* Handle page navigation via BOOTSEL button */
static void handle_page_navigation(void) {
    static uint64_t last_button_press = 0;
    uint64_t now = get_time_ms();
    
    /* Debounce: 200ms */
    if (now - last_button_press < 200) {
        return;
    }
    
    /* Note: Reading BOOTSEL requires special handling on RP2040
     * For now, we'll cycle through pages automatically */
    current_page = (current_page + 1) % 3;
    last_button_press = now;
}

/* Main application */
int main() {
    stdio_init_all();
    
    /* Initialize LED for debugging */
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    printf("\n========================================\n");
    printf("PlugSafe - USB Threat Detector\n");
    printf("========================================\n\n");
    
    /* Initialize I2C for OLED */
    printf("Initializing OLED display...\n");
    oled_i2c_t i2c = {
        .i2c = I2C_PORT,
        .sda_pin = I2C_SDA_PIN,
        .scl_pin = I2C_SCL_PIN,
        .baudrate = I2C_BAUDRATE,
        .address = OLED_ADDRESS
    };
    
    if (!oled_i2c_init(&i2c)) {
        printf("ERROR: I2C initialization failed\n");
        while (1) {
            gpio_put(LED_PIN, 1);
            sleep_ms(100);
            gpio_put(LED_PIN, 0);
            sleep_ms(100);
        }
    }
    printf("I2C initialized successfully\n");
    
    /* Initialize OLED driver */
    oled_driver_t driver;
    if (!oled_driver_init(&driver, OLED_DISPLAY_SSD1306, &i2c)) {
        printf("ERROR: OLED driver initialization failed\n");
        while (1) {
            gpio_put(LED_PIN, 1);
            sleep_ms(200);
            gpio_put(LED_PIN, 0);
            sleep_ms(200);
        }
    }
    printf("OLED driver initialized\n");
    
    /* Initialize display framebuffer */
    oled_display_t display;
    if (!oled_display_init(&display, &driver)) {
        printf("ERROR: Display initialization failed\n");
        while (1) {
            gpio_put(LED_PIN, 1);
            sleep_ms(300);
            gpio_put(LED_PIN, 0);
            sleep_ms(300);
        }
    }
    printf("Display initialized\n");
    
    /* Get font for text rendering */
    const oled_font_t *font = oled_get_font_5x7();
    
    /* Initialize USB host */
    printf("\nInitializing USB host...\n");
    if (!usb_host_init()) {
        printf("ERROR: USB host initialization failed\n");
        while (1) {
            gpio_put(LED_PIN, 1);
            sleep_ms(500);
            gpio_put(LED_PIN, 0);
            sleep_ms(500);
        }
    }
    
    /* Initialize threat analyzer */
    printf("\nInitializing threat analyzer...\n");
    threat_analyzer_init();
    
    /* Initialize HID monitor */
    printf("\nInitializing HID monitor...\n");
    hid_monitor_init();
    
    /* Show startup screen */
    printf("\nStartup complete. Entering main loop...\n");
    printf("=====================================\n\n");
    
    oled_display_clear(&display);
    oled_draw_string(&display, 20, 20, "PlugSafe", font, true);
    oled_draw_string(&display, 15, 35, "Ready", font, true);
    oled_display_flush(&display);
    
    /* Blink LED to indicate startup complete */
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    last_display_update_ms = get_time_ms();
    last_usb_poll_ms = get_time_ms();
    
    /* ===== MAIN LOOP ===== */
    while (true) {
        uint64_t now = get_time_ms();
        
        /* USB polling */
        if (now - last_usb_poll_ms >= USB_HOST_POLL_INTERVAL_MS) {
            usb_host_task();
            last_usb_poll_ms = now;
        }
        
        /* Update OLED display */
        if (now - last_display_update_ms >= DISPLAY_UPDATE_INTERVAL_MS) {
            /* Draw appropriate page */
            switch (current_page) {
                case 1:
                    draw_page_details(&display, font);
                    break;
                case 2:
                    draw_page_hid_monitor(&display, font);
                    break;
                default:
                    draw_page_status(&display, font);
            }
            
            /* Flush to hardware */
            if (!oled_display_flush(&display)) {
                printf("ERROR: Failed to update display\n");
            }
            
            last_display_update_ms = now;
            
            /* Toggle LED slowly to show it's running */
            gpio_put(LED_PIN, (now / 500) % 2);
        }
        
        /* Handle page navigation (for future BOOTSEL implementation) */
        handle_page_navigation();
        
        /* Small sleep to prevent 100% CPU */
        sleep_us(100);
    }
    
    return 0;
}
