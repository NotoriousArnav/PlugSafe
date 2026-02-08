/*
 * OLED Graphics Primitives
 * Pixel, line, and shape drawing
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef OLED_GRAPHICS_H
#define OLED_GRAPHICS_H

#include <stdbool.h>
#include "oled_display.h"

/* Draw single pixel */
void oled_draw_pixel(oled_display_t *display, int x, int y, bool on);

/* Read pixel */
bool oled_get_pixel(oled_display_t *display, int x, int y);

/* Draw horizontal line */
void oled_draw_hline(oled_display_t *display, int x, int y, int len, bool on);

/* Draw vertical line */
void oled_draw_vline(oled_display_t *display, int x, int y, int len, bool on);

/* Draw arbitrary line (Bresenham) */
void oled_draw_line(oled_display_t *display, int x0, int y0, int x1, int y1, bool on);

/* Draw rectangle */
void oled_draw_rect(oled_display_t *display, int x, int y, int w, int h, 
                    bool fill, bool on);

/* Draw circle */
void oled_draw_circle(oled_display_t *display, int cx, int cy, int r, 
                      bool fill, bool on);

/* Draw bitmap */
void oled_draw_bitmap(oled_display_t *display, int x, int y, 
                      const uint8_t *bitmap, int w, int h);

#endif /* OLED_GRAPHICS_H */
