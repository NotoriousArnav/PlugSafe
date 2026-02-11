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

#ifndef OLED_TEXT_H
#define OLED_TEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "oled_display.h"

/* Font definition */
typedef struct {
    const uint8_t *data;
    uint16_t width;  /* Changed from uint8_t to uint16_t - max font size 475 bytes */
    uint8_t height;
    uint8_t char_width;
    char start_char;
    char end_char;
} oled_font_t;

/* Draw single character */
int oled_draw_char(oled_display_t *display, int x, int y, 
                   char c, const oled_font_t *font, bool on);

/* Draw null-terminated string */
int oled_draw_string(oled_display_t *display, int x, int y, 
                     const char *str, const oled_font_t *font, bool on);

/* Measure string width */
int oled_measure_string(const char *str, const oled_font_t *font);

#endif /* OLED_TEXT_H */
