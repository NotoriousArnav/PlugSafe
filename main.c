/*
 * PlugSafe - Malicious USB Detector with OLED Display
 * Demonstration of OLED driver module
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "pico/stdlib.h"
#include "oled_i2c.h"
#include "oled_driver.h"
#include "oled_display.h"
#include "oled_graphics.h"
#include "oled_text.h"
#include "oled_font.h"

/* GPIO pins for LED */
#define LED_PIN 25

/* I2C pins for OLED display */
#define I2C_SDA_PIN 0
#define I2C_SCL_PIN 1
#define I2C_PORT i2c0
#define I2C_BAUDRATE 400000

/* OLED display address */
#define OLED_ADDRESS 0x3C

int main() {
    stdio_init_all();
    
    /* Initialize LED for debugging */
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    printf("PlugSafe - USB Threat Detector\n");
    printf("Initializing OLED display...\n");
    
    /* Initialize I2C for OLED */
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
    printf("OLED driver initialized successfully\n");
    
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
    printf("Display initialized successfully\n");
    
    /* Get font for text rendering */
    const oled_font_t *font = oled_get_font_5x7();
    
    printf("Drawing on display...\n");
    
    /* Clear display */
    oled_display_clear(&display);
    
    /* Draw title */
    oled_draw_string(&display, 10, 0, "PlugSafe", font, true);
    
    /* Draw separator line */
    oled_draw_hline(&display, 0, 10, 128, true);
    
    /* Draw status text */
    oled_draw_string(&display, 5, 15, "USB Detector", font, true);
    oled_draw_string(&display, 5, 25, "Ready", font, true);
    
    /* Draw a simple box */
    oled_draw_rect(&display, 5, 35, 118, 25, false, true);
    
    /* Draw some decorative elements */
    oled_draw_circle(&display, 20, 50, 5, false, true);
    oled_draw_circle(&display, 108, 50, 5, false, true);
    
    /* Update display hardware */
    if (!oled_display_flush(&display)) {
        printf("ERROR: Failed to flush display\n");
    } else {
        printf("Display updated successfully\n");
    }
    
    /* Blink LED to indicate success */
    for (int i = 0; i < 5; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    /* Main loop */
    int counter = 0;
    while (true) {
        /* Simulate USB detection status changes */
        counter++;
        
        oled_display_clear(&display);
        
        /* Draw header */
        oled_draw_string(&display, 10, 0, "PlugSafe", font, true);
        oled_draw_hline(&display, 0, 10, 128, true);
        
        /* Draw status based on counter */
        if ((counter / 10) % 2 == 0) {
            /* Show safe status */
            oled_draw_string(&display, 20, 20, "No Threats", font, true);
            oled_draw_circle(&display, 64, 45, 8, true, true);  /* Green indicator */
        } else {
            /* Show scanning status */
            oled_draw_string(&display, 15, 20, "Scanning...", font, true);
            oled_draw_rect(&display, 50, 40, 28, 15, false, true);  /* Progress box */
        }
        
        /* Draw counter */
        char counter_str[16];
        snprintf(counter_str, sizeof(counter_str), "Count: %d", counter);
        oled_draw_string(&display, 5, 55, counter_str, font, true);
        
        oled_display_flush(&display);
        
        /* Toggle LED */
        gpio_put(LED_PIN, (counter / 20) % 2);
        
        sleep_ms(100);
    }
    
    return 0;
}
