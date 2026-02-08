/*
 * OLED Font Definitions
 * Bitmap font data and access functions
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef OLED_FONT_H
#define OLED_FONT_H

#include "oled_text.h"

/* Get built-in fonts */
const oled_font_t *oled_get_font_5x7(void);
const oled_font_t *oled_get_font_8x8(void);

#endif /* OLED_FONT_H */
