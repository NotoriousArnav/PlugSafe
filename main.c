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
#include "usb_detector.h"
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
     
     /* Initialize USB detector */
     printf("Initializing USB detector...\n");
     usb_detector_init();
     printf("USB detector initialized\n");
     
     /* Initialize threat analyzer */
     printf("Initializing threat analyzer...\n");
     threat_analyzer_init();
     printf("Threat analyzer initialized\n\n");
     
      /* Get font for text rendering */
     const oled_font_t *font = oled_get_font_5x7();
     printf("Font info: width=%d, height=%d, char_width=%d, start=0x%02X, end=0x%02X\n",
            font->width, font->height, font->char_width, font->start_char, font->end_char);
  
     /* Simple Hello World display */
     oled_display_clear(&display);
     printf("Drawing 'Hello World' at (10, 20)\n");
     int drawn_width = oled_draw_string(&display, 10, 20, "Hello World", font, true);
     printf("String drawn, width returned: %d\n", drawn_width);
     
     printf("Flushing to display...\n");
     bool flush_result = oled_display_flush(&display);
     printf("Flush result: %s\n", flush_result ? "SUCCESS" : "FAILED");
    
     /* Blink LED to indicate startup complete */
     for (int i = 0; i < 3; i++) {
         gpio_put(LED_PIN, 1);
         sleep_ms(100);
         gpio_put(LED_PIN, 0);
         sleep_ms(100);
     }
    
     printf("\nEntering main event loop...\n");
     printf("Display will refresh every %d ms\n", DISPLAY_UPDATE_INTERVAL_MS);
     printf("USB polling every %d ms\n\n", USB_HOST_POLL_INTERVAL_MS);
     
     /* Main event loop */
     while (1) {
         uint64_t now_ms = time_us_64() / 1000;
         
         /* USB Host polling (every 10ms) */
         if (now_ms - last_usb_poll_ms >= USB_HOST_POLL_INTERVAL_MS) {
             last_usb_poll_ms = now_ms;
             /* TODO: Call usb_host_poll() here */
         }
         
         /* Display refresh (every 500ms) */
         if (now_ms - last_display_update_ms >= DISPLAY_UPDATE_INTERVAL_MS) {
             last_display_update_ms = now_ms;
             
             /* Clear and redraw display based on current page */
             oled_display_clear(&display);
             
             /* Draw page content */
             switch (current_page) {
                 case 0:
                     /* Status page */
                     oled_draw_string(&display, 5, 5, "=== PlugSafe ===", font, true);
                     oled_draw_string(&display, 5, 15, "Status: MONITORING", font, true);
                     oled_draw_string(&display, 5, 25, "USB Devices: 0", font, true);
                     oled_draw_string(&display, 5, 35, "Threat Level: SAFE", font, true);
                     oled_draw_string(&display, 5, 50, "Uptime: ", font, true);
                     break;
                     
                 case 1:
                     /* Device details page */
                     oled_draw_string(&display, 5, 5, "No USB Devices", font, true);
                     oled_draw_string(&display, 5, 15, "Connected", font, true);
                     break;
                     
                 case 2:
                     /* HID monitor page */
                     oled_draw_string(&display, 5, 5, "HID Monitor", font, true);
                     oled_draw_string(&display, 5, 15, "Keystroke Rate: 0", font, true);
                     oled_draw_string(&display, 5, 25, "Threat: NONE", font, true);
                     break;
                     
                 default:
                     current_page = 0;
                     break;
             }
             
             /* Flush to display */
             oled_display_flush(&display);
             
             /* Blink LED every second (toggle on display update at 2Hz) */
             gpio_put(LED_PIN, (now_ms / 500) % 2);
         }
         
         /* Small sleep to prevent busy-waiting */
         sleep_ms(1);
     }
    
     return 0;
 }
