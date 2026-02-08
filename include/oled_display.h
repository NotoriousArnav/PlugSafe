/*
 * OLED Display Framebuffer Management
 * Manages framebuffer and display abstraction
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "oled_config.h"
#include "oled_driver.h"

/* Display with framebuffer */
typedef struct {
    oled_driver_t *driver;
    uint8_t *buffer;
    uint8_t width;
    uint8_t height;
    bool dirty;
} oled_display_t;

/* Initialize display */
bool oled_display_init(oled_display_t *display, oled_driver_t *driver);

/* Cleanup display */
void oled_display_deinit(oled_display_t *display);

/* Clear display (all pixels off) */
void oled_display_clear(oled_display_t *display);

/* Invert all pixels in framebuffer */
void oled_display_invert(oled_display_t *display, bool invert);

/* Send framebuffer to display hardware */
bool oled_display_flush(oled_display_t *display);

/* Get framebuffer pointer */
uint8_t *oled_display_get_buffer(oled_display_t *display);

/* Get framebuffer size */
size_t oled_display_get_buffer_size(oled_display_t *display);

/* Get display dimensions */
void oled_display_get_dimensions(oled_display_t *display, 
                                 uint8_t *width, uint8_t *height);

#endif /* OLED_DISPLAY_H */
