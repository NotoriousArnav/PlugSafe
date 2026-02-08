/*
 * OLED Display Driver (SSD1306/SH1106)
 * Controller-specific command sequences
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "oled_config.h"
#include "oled_i2c.h"

/* OLED Driver state */
typedef struct {
    oled_i2c_t *i2c;
    oled_display_type_e type;
    uint8_t width;
    uint8_t height;
    bool is_on;
    uint8_t contrast;
    uint8_t page_start;
} oled_driver_t;

/* SSD1306 Command Set */
typedef enum {
    SSD1306_CMD_SETCONTRAST = 0x81,
    SSD1306_CMD_DISPLAYRAM = 0xA4,
    SSD1306_CMD_DISPLAYNORMAL = 0xA6,
    SSD1306_CMD_DISPLAYINVERT = 0xA7,
    SSD1306_CMD_DISPLAYOFF = 0xAE,
    SSD1306_CMD_DISPLAYON = 0xAF,
    SSD1306_CMD_SETPAGE = 0xB0,
    SSD1306_CMD_SETLOWCOL = 0x00,
    SSD1306_CMD_SETHIGHCOL = 0x10,
} ssd1306_cmd_e;

/* Initialize OLED driver */
bool oled_driver_init(oled_driver_t *driver, 
                      oled_display_type_e type, 
                      oled_i2c_t *i2c);

/* Set page address (row) */
bool oled_driver_set_page(oled_driver_t *driver, uint8_t page);

/* Set column address (x) */
bool oled_driver_set_column(oled_driver_t *driver, uint8_t col);

/* Write pixel data at current position */
bool oled_driver_write_pixel_data(oled_driver_t *driver, 
                                  const uint8_t *data, size_t len);

/* Power control */
bool oled_driver_power_on(oled_driver_t *driver);
bool oled_driver_power_off(oled_driver_t *driver);

/* Contrast control */
bool oled_driver_set_contrast(oled_driver_t *driver, uint8_t contrast);

/* Invert display */
bool oled_driver_set_invert(oled_driver_t *driver, bool invert);

#endif /* OLED_DRIVER_H */
