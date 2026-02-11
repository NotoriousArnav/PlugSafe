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
#define DISPLAY_UPDATE_INTERVAL_MS 200
#define USB_HOST_POLL_INTERVAL_MS  10

/* Display page enumeration for state management */
typedef enum {
    DISPLAY_PAGE_WELCOME = 0,      /* Welcome/waiting screen */
    DISPLAY_PAGE_DEVICE_INFO = 1,  /* Device details screen */
    DISPLAY_MODE_COUNT = 2
} display_page_t;

/* Display mode enumeration - switches between VID/PID and Manufacturer */
typedef enum {
    DISPLAY_MODE_VID_PID = 0,        /* Show VID/PID and USB Class */
    DISPLAY_MODE_MANUFACTURER = 1    /* Show Manufacturer/Product strings */
} display_mode_t;

/* Current display page and mode */
static display_page_t current_page = DISPLAY_PAGE_WELCOME;
static display_mode_t current_mode = DISPLAY_MODE_VID_PID;

/* Time tracking for display updates */
static uint64_t last_display_update_ms = 0;
static uint64_t last_usb_poll_ms = 0;
static uint64_t last_bootsel_check_ms = 0;

/* Track last device count for edge detection */
static uint8_t last_device_count = 0;

/* LED control with adaptive blinking based on device state */
static uint64_t last_led_update_ms = 0;

/* BOOTSEL button debouncing */
#define BOOTSEL_DEBOUNCE_MS 200
static bool bootsel_pressed_prev = false;

/* ============================================================================
 * DISPLAY HELPER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Draw the welcome screen (waiting for device)
 */
static void draw_welcome_screen(oled_display_t *display, const oled_font_t *font) {
    oled_display_clear(display);
    
    /* Title */
    oled_draw_string(display, 10, 5,   "=== PlugSafe ===", font, true);
    
    /* Main message */
    oled_draw_string(display, 8, 20,  "Insert USB Device", font, true);
    
    /* Instructions */
    oled_draw_string(display, 5, 32,  "to start monitoring", font, true);
    
    /* Separator */
    oled_draw_string(display, 5, 42,  "---", font, true);
    
    /* Status indicator */
    oled_draw_string(display, 8, 52,  "Waiting...", font, true);
}

/**
 * @brief Draw the device information screen with dual mode support
 * Mode 1: VID/PID and USB Class (default)
 * Mode 2: Manufacturer/Product/Serial (press BOOTSEL to toggle)
 */
static void draw_device_screen(oled_display_t *display, const oled_font_t *font) {
    oled_display_clear(display);
    
    uint8_t device_count = usb_get_device_count();
    
    /* If no devices, show waiting message */
    if (device_count == 0) {
        draw_welcome_screen(display, font);
        return;
    }
    
    /* Get device information */
    usb_device_info_t *dev = usb_get_device_at_index(0);
    if (!dev) {
        oled_draw_string(display, 5, 20, "Error reading device", font, true);
        return;
    }
    
    /* Title */
    oled_draw_string(display, 10, 2,   "Device Detected!", font, true);
    
    char buf[28];
    
    if (current_mode == DISPLAY_MODE_VID_PID) {
        /* Mode 1: VID/PID display */
        
        /* Device name/product (truncated) */
        char product_short[20];
        strncpy(product_short, dev->product[0] ? dev->product : "Unknown Device", 19);
        product_short[19] = '\0';
        oled_draw_string(display, 5, 12, product_short, font, true);
        
        /* VID/PID */
        snprintf(buf, sizeof(buf), "VID:0x%04X PID:0x%04X", dev->vid, dev->pid);
        oled_draw_string(display, 5, 22, buf, font, true);
        
        /* USB Class and HID indicator */
        const char *type_str = "STD";
        if (dev->is_hid) {
            if (dev->hid_protocol == 1) type_str = "KBD";
            else if (dev->hid_protocol == 2) type_str = "MOUSE";
            else type_str = "HID";
        }
        snprintf(buf, sizeof(buf), "Class: 0x%02X %s", dev->usb_class, type_str);
        oled_draw_string(display, 5, 32, buf, font, true);
        
        /* Threat level */
        device_threat_t *threat = threat_get_device_at_index(0);
        const char *threat_str = "SAFE";
        if (threat) {
            switch (threat->threat_level) {
                case THREAT_MALICIOUS:
                    threat_str = "MALICIOUS!!!";
                    break;
                case THREAT_POTENTIALLY_UNSAFE:
                    threat_str = "CAUTION";
                    break;
                default:
                    threat_str = "SAFE";
            }
        }
        
        snprintf(buf, sizeof(buf), "Threat: %s", threat_str);
        oled_draw_string(display, 5, 42, buf, font, true);
        
        /* Show live keystroke rate for HID devices, mode indicator otherwise */
        if (dev->is_hid) {
            uint32_t rate = threat ? threat->hid_reports_per_sec : 0;
            snprintf(buf, sizeof(buf), "Rate:%u k/s", rate);
            oled_draw_string(display, 0, 56, buf, font, true);
        } else {
            oled_draw_string(display, 0, 56, "Mode: IDs", font, true);
        }
        oled_draw_string(display, 80, 56, "BOOTSEL", font, true);
        
    } else {
        /* Mode 2: Manufacturer/Product/Serial display */
        
        /* Manufacturer (truncated) */
        char mfg_short[18];
        strncpy(mfg_short, dev->manufacturer[0] ? dev->manufacturer : "Unknown", 17);
        mfg_short[17] = '\0';
        oled_draw_string(display, 5, 12, mfg_short, font, true);
        
        /* Product (truncated) */
        char prod_short[18];
        strncpy(prod_short, dev->product[0] ? dev->product : "Unknown Device", 17);
        prod_short[17] = '\0';
        oled_draw_string(display, 5, 22, prod_short, font, true);
        
        /* Serial (truncated) */
        char serial_short[18];
        strncpy(serial_short, dev->serial[0] ? dev->serial : "No Serial", 17);
        serial_short[17] = '\0';
        oled_draw_string(display, 5, 32, serial_short, font, true);
        
        /* Threat level */
        device_threat_t *threat = threat_get_device_at_index(0);
        const char *threat_str = "SAFE";
        if (threat) {
            switch (threat->threat_level) {
                case THREAT_MALICIOUS:
                    threat_str = "MALICIOUS!!!";
                    break;
                case THREAT_POTENTIALLY_UNSAFE:
                    threat_str = "CAUTION";
                    break;
                default:
                    threat_str = "SAFE";
            }
        }
        
        snprintf(buf, sizeof(buf), "Threat: %s", threat_str);
        oled_draw_string(display, 5, 42, buf, font, true);
        
        /* Show live keystroke rate for HID devices, mode indicator otherwise */
        if (dev->is_hid) {
            uint32_t rate = threat ? threat->hid_reports_per_sec : 0;
            snprintf(buf, sizeof(buf), "Rate:%u k/s", rate);
            oled_draw_string(display, 0, 56, buf, font, true);
        } else {
            oled_draw_string(display, 0, 56, "Mode: Strings", font, true);
        }
        oled_draw_string(display, 80, 56, "BOOTSEL", font, true);
    }
}

/**
 * @brief Draw the USB Hub warning page
 */
static void draw_hub_warning_page(oled_display_t *display, const oled_font_t *font) {
    oled_display_clear(display);
    
    oled_draw_string(display, 5, 2,   "!!! WARNING !!!", font, true);
    oled_draw_string(display, 5, 12,  "USB HUB DETECTED", font, true);
    oled_draw_string(display, 5, 22,  "", font, true);
    oled_draw_string(display, 5, 32,  "Please disconnect", font, true);
    oled_draw_string(display, 5, 42,  "hub and connect", font, true);
    oled_draw_string(display, 5, 52,  "device directly.", font, true);
}

/* ============================================================================
 * MAIN APPLICATION
 * ============================================================================ */

int main() {
    stdio_init_all();
    
    /* Initialize LED for debugging */
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    /* Initialize BOOTSEL button (GPIO 24, active low) */
    gpio_init(BOOTSEL_PIN);
    gpio_set_dir(BOOTSEL_PIN, GPIO_IN);
    gpio_pull_up(BOOTSEL_PIN);
    
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
    
    /* Initialize USB Host (TinyUSB active enumeration) */
    printf("Initializing USB host...\n");
    if (!usb_host_init()) {
        printf("ERROR: USB host initialization failed\n");
        /* Continue anyway - USB might not be essential */
    }
    printf("USB host initialized\n");
    
    /* Initialize threat analyzer */
    printf("Initializing threat analyzer...\n");
    threat_analyzer_init();
    printf("Threat analyzer initialized\n\n");
    
    /* Get font for text rendering */
    const oled_font_t *font = oled_get_font_5x7();
    printf("Font info: width=%d, height=%d, char_width=%d, start=0x%02X, end=0x%02X\n",
           font->width, font->height, font->char_width, font->start_char, font->end_char);
    
    /* Display startup message */
    oled_display_clear(&display);
    oled_draw_string(&display, 5, 20, "PlugSafe Booting...", font, true);
    oled_draw_string(&display, 5, 40, "The Protection your PC deserves", font, true);
    oled_display_flush(&display);
    sleep_ms(2000);
    
    /* Blink LED to indicate startup complete */
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
     printf("\nEntering main event loop...\n");
    printf("Display will refresh every %d ms\n", DISPLAY_UPDATE_INTERVAL_MS);
    printf("USB polling every %d ms\n", USB_HOST_POLL_INTERVAL_MS);
    printf("Press BOOTSEL button to toggle display mode (VID/PID <-> Manufacturer)\n\n");
    
    /* Main event loop */
    while (1) {
        uint64_t now_ms = time_us_64() / 1000;
        
        /* BOOTSEL button handling for display mode toggle (every 200ms debounce) */
        if (now_ms - last_bootsel_check_ms >= BOOTSEL_DEBOUNCE_MS) {
            last_bootsel_check_ms = now_ms;
            
            /* Read BOOTSEL button state (GPIO 24, active low) */
            bool bootsel_pressed = !gpio_get(BOOTSEL_PIN);
            
            /* Detect rising edge (button pressed) */
            if (bootsel_pressed && !bootsel_pressed_prev) {
                /* Toggle display mode */
                current_mode = (current_mode == DISPLAY_MODE_VID_PID) ? 
                               DISPLAY_MODE_MANUFACTURER : DISPLAY_MODE_VID_PID;
                printf("[BUTTON] Display mode toggled to: %s\n",
                       current_mode == DISPLAY_MODE_VID_PID ? "VID/PID" : "Manufacturer");
                /* Force immediate display update */
                last_display_update_ms = 0;
            }
            bootsel_pressed_prev = bootsel_pressed;
        }
        
        /* USB Host polling (every 10ms) */
        if (now_ms - last_usb_poll_ms >= USB_HOST_POLL_INTERVAL_MS) {
            last_usb_poll_ms = now_ms;
            usb_host_task();
        }
        
        /* Edge detection: check if device count changed */
        uint8_t current_device_count = usb_get_device_count();
        bool device_state_changed = (current_device_count != last_device_count);
        
        if (device_state_changed) {
            last_device_count = current_device_count;
            /* Force immediate display update on device connection/disconnection */
            last_display_update_ms = 0;
        }
        
        /* Display refresh (every 200ms or on state change) */
        if (now_ms - last_display_update_ms >= DISPLAY_UPDATE_INTERVAL_MS) {
            last_display_update_ms = now_ms;
            
            /* Automatic page switching based on device connection status */
            uint8_t device_count = usb_get_device_count();
            
            /* Check if hub is connected - if so, always show warning page */
            if (usb_is_hub_connected()) {
                draw_hub_warning_page(&display, font);
                current_page = DISPLAY_PAGE_WELCOME;  /* Reset page when hub detected */
            } else {
                /* Switch pages based on device presence */
                if (device_count > 0) {
                    /* Device connected - show device info */
                    current_page = DISPLAY_PAGE_DEVICE_INFO;
                    draw_device_screen(&display, font);
                } else {
                    /* No device - show welcome screen */
                    current_page = DISPLAY_PAGE_WELCOME;
                    draw_welcome_screen(&display, font);
                }
            }
            
            /* Flush to display */
            oled_display_flush(&display);
            
            /* Adaptive LED blinking based on device state:
             * - Device connected: Fast blink (200ms ON, 200ms OFF)
             * - No device: Slow blink (500ms ON, 500ms OFF)
             */
            if (now_ms - last_led_update_ms >= 100) {
                last_led_update_ms = now_ms;
                
                if (current_device_count > 0) {
                    /* Device connected - fast blink (200ms half period) */
                    gpio_put(LED_PIN, (now_ms / 200) % 2);
                } else {
                    /* No device - slow blink (500ms half period) */
                    gpio_put(LED_PIN, (now_ms / 500) % 2);
                }
            }
        }
        
        /* Small sleep to prevent busy-waiting */
        sleep_ms(1);
    }
    
    return 0;
}
