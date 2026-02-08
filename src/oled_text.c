/*
 * OLED Text Rendering
 * Character and string drawing with bitmap fonts
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "oled_text.h"
#include "oled_graphics.h"

int oled_draw_char(oled_display_t *display, int x, int y, 
                   char c, const oled_font_t *font, bool on)
{
    if (!display || !font) {
        return 0;
    }

    /* Check if character is in font range */
    if (c < font->start_char || c > font->end_char) {
        return 0;
    }

    /* Calculate character offset in font data */
    int char_index = c - font->start_char;
    int char_offset = char_index * font->char_width;

    /* Draw character bitmap */
    for (int row = 0; row < font->height; row++) {
        for (int col = 0; col < font->char_width; col++) {
            int byte_idx = char_offset + col;
            if (byte_idx >= font->width) {
                break;
            }

            uint8_t byte_val = font->data[byte_idx];
            bool pixel = (byte_val & (1 << row)) != 0;

            oled_draw_pixel(display, x + col, y + row, pixel ? on : !on);
        }
    }

    return font->char_width;
}

int oled_draw_string(oled_display_t *display, int x, int y, 
                     const char *str, const oled_font_t *font, bool on)
{
    if (!display || !str || !font) {
        return 0;
    }

    int total_width = 0;
    int current_x = x;

    for (const char *p = str; *p != '\0'; p++) {
        int char_width = oled_draw_char(display, current_x, y, *p, font, on);
        current_x += char_width;
        total_width += char_width;
    }

    return total_width;
}

int oled_measure_string(const char *str, const oled_font_t *font)
{
    if (!str || !font) {
        return 0;
    }

    int total_width = 0;

    for (const char *p = str; *p != '\0'; p++) {
        if (*p >= font->start_char && *p <= font->end_char) {
            total_width += font->char_width;
        }
    }

    return total_width;
}
