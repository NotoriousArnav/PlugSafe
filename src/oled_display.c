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

#include "oled_display.h"
#include <stdlib.h>
#include <string.h>

bool oled_display_init(oled_display_t *display, oled_driver_t *driver)
{
    if (!display || !driver) {
        return false;
    }

    display->driver = driver;
    display->width = driver->width;
    display->height = driver->height;
    display->dirty = true;

    /* Allocate framebuffer */
    display->buffer = (uint8_t *)malloc(OLED_BUFFER_SIZE);
    if (!display->buffer) {
        return false;
    }

    /* Clear framebuffer */
    memset(display->buffer, 0, OLED_BUFFER_SIZE);

    return true;
}

void oled_display_deinit(oled_display_t *display)
{
    if (!display) {
        return;
    }

    if (display->buffer) {
        free(display->buffer);
        display->buffer = NULL;
    }
}

void oled_display_clear(oled_display_t *display)
{
    if (!display || !display->buffer) {
        return;
    }

    memset(display->buffer, 0, OLED_BUFFER_SIZE);
    display->dirty = true;
}

void oled_display_invert(oled_display_t *display, bool invert)
{
    if (!display || !display->buffer) {
        return;
    }

    for (int i = 0; i < OLED_BUFFER_SIZE; i++) {
        if (invert) {
            display->buffer[i] = ~display->buffer[i];
        }
    }
    
    display->dirty = true;
}

bool oled_display_flush(oled_display_t *display)
{
    if (!display || !display->buffer || !display->driver) {
        return false;
    }

    /* Send each page to display */
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        if (!oled_driver_set_page(display->driver, page)) {
            return false;
        }

        if (!oled_driver_set_column(display->driver, 0)) {
            return false;
        }

        /* Send 128 bytes for this page */
        uint8_t *page_data = display->buffer + (page * OLED_WIDTH);
        if (!oled_driver_write_pixel_data(display->driver, page_data, OLED_WIDTH)) {
            return false;
        }
    }

    display->dirty = false;
    return true;
}

uint8_t *oled_display_get_buffer(oled_display_t *display)
{
    if (!display) {
        return NULL;
    }

    return display->buffer;
}

size_t oled_display_get_buffer_size(oled_display_t *display)
{
    (void)display;
    return OLED_BUFFER_SIZE;
}

void oled_display_get_dimensions(oled_display_t *display, 
                                 uint8_t *width, uint8_t *height)
{
    if (!display) {
        return;
    }

    if (width) {
        *width = display->width;
    }

    if (height) {
        *height = display->height;
    }
}
