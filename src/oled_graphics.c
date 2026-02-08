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

#include "oled_graphics.h"
#include <stdlib.h>

/* Helper: check if pixel is in bounds */
static bool is_in_bounds(oled_display_t *display, int x, int y)
{
    return (x >= 0 && x < display->width && y >= 0 && y < display->height);
}

void oled_draw_pixel(oled_display_t *display, int x, int y, bool on)
{
    if (!display || !display->buffer) {
        return;
    }

    /* Bounds check and clip */
    if (!is_in_bounds(display, x, y)) {
        return;
    }

    uint16_t page = y / 8;
    uint8_t bit = y % 8;
    uint16_t index = page * OLED_WIDTH + x;

    if (on) {
        display->buffer[index] |= (1 << bit);
    } else {
        display->buffer[index] &= ~(1 << bit);
    }

    display->dirty = true;
}

bool oled_get_pixel(oled_display_t *display, int x, int y)
{
    if (!display || !display->buffer) {
        return false;
    }

    if (!is_in_bounds(display, x, y)) {
        return false;
    }

    uint16_t page = y / 8;
    uint8_t bit = y % 8;
    uint16_t index = page * OLED_WIDTH + x;

    return (display->buffer[index] & (1 << bit)) != 0;
}

void oled_draw_hline(oled_display_t *display, int x, int y, int len, bool on)
{
    if (!display) {
        return;
    }

    for (int i = 0; i < len; i++) {
        oled_draw_pixel(display, x + i, y, on);
    }
}

void oled_draw_vline(oled_display_t *display, int x, int y, int len, bool on)
{
    if (!display) {
        return;
    }

    for (int i = 0; i < len; i++) {
        oled_draw_pixel(display, x, y + i, on);
    }
}

/* Bresenham line algorithm */
void oled_draw_line(oled_display_t *display, int x0, int y0, int x1, int y1, bool on)
{
    if (!display) {
        return;
    }

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        oled_draw_pixel(display, x0, y0, on);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void oled_draw_rect(oled_display_t *display, int x, int y, int w, int h, 
                    bool fill, bool on)
{
    if (!display) {
        return;
    }

    if (fill) {
        /* Filled rectangle */
        for (int yy = 0; yy < h; yy++) {
            for (int xx = 0; xx < w; xx++) {
                oled_draw_pixel(display, x + xx, y + yy, on);
            }
        }
    } else {
        /* Rectangle outline */
        oled_draw_hline(display, x, y, w, on);
        oled_draw_hline(display, x, y + h - 1, w, on);
        oled_draw_vline(display, x, y, h, on);
        oled_draw_vline(display, x + w - 1, y, h, on);
    }
}

/* Midpoint circle algorithm */
void oled_draw_circle(oled_display_t *display, int cx, int cy, int r, 
                      bool fill, bool on)
{
    if (!display) {
        return;
    }

    int x = r;
    int y = 0;
    int d = 3 - 2 * r;

    while (x >= y) {
        if (fill) {
            /* Draw horizontal lines for filled circle */
            oled_draw_hline(display, cx - x, cy + y, 2 * x + 1, on);
            oled_draw_hline(display, cx - x, cy - y, 2 * x + 1, on);
            oled_draw_hline(display, cx - y, cy + x, 2 * y + 1, on);
            oled_draw_hline(display, cx - y, cy - x, 2 * y + 1, on);
        } else {
            /* Draw circle outline */
            oled_draw_pixel(display, cx + x, cy + y, on);
            oled_draw_pixel(display, cx - x, cy + y, on);
            oled_draw_pixel(display, cx + x, cy - y, on);
            oled_draw_pixel(display, cx - x, cy - y, on);
            oled_draw_pixel(display, cx + y, cy + x, on);
            oled_draw_pixel(display, cx - y, cy + x, on);
            oled_draw_pixel(display, cx + y, cy - x, on);
            oled_draw_pixel(display, cx - y, cy - x, on);
        }

        if (d < 0) {
            d = d + 4 * y + 6;
        } else {
            d = d + 4 * (y - x) + 10;
            x--;
        }
        y++;
    }
}

void oled_draw_bitmap(oled_display_t *display, int x, int y, 
                      const uint8_t *bitmap, int w, int h)
{
    if (!display || !bitmap) {
        return;
    }

    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            uint8_t byte_idx = (yy / 8) * w + xx;
            uint8_t bit = yy % 8;
            
            bool pixel = (bitmap[byte_idx] & (1 << bit)) != 0;
            oled_draw_pixel(display, x + xx, y + yy, pixel);
        }
    }
}
