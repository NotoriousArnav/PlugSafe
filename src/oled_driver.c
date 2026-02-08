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

#include "oled_driver.h"
#include "pico/time.h"

/* Forward declarations */
static bool oled_driver_init_ssd1306(oled_driver_t *driver);
static bool oled_driver_init_sh1106(oled_driver_t *driver);

bool oled_driver_init(oled_driver_t *driver, 
                      oled_display_type_e type, 
                      oled_i2c_t *i2c)
{
    if (!driver || !i2c) {
        return false;
    }

    driver->i2c = i2c;
    driver->type = type;
    driver->width = OLED_WIDTH;
    driver->height = OLED_HEIGHT;
    driver->is_on = false;
    driver->contrast = 127;
    driver->page_start = 0;

    if (type == OLED_DISPLAY_SSD1306) {
        return oled_driver_init_ssd1306(driver);
    } else if (type == OLED_DISPLAY_SH1106) {
        return oled_driver_init_sh1106(driver);
    }

    return false;
}

static bool oled_driver_init_ssd1306(oled_driver_t *driver)
{
    uint8_t init_cmds[] = {
        0xAE,           /* Display OFF (0xAE) */
        0xD5, 0x80,     /* Set display clock divide ratio */
        0xA8, 0x3F,     /* Set multiplex ratio (64) */
        0xD3, 0x00,     /* Set display offset */
        0x40,           /* Set start line */
        0x8D, 0x14,     /* Enable charge pump */
        0x20, 0x00,     /* Set memory addressing mode (horizontal) */
        0xA1,           /* Set segment remap (rotate 180°) */
        0xC8,           /* Set COM output direction */
        0xDA, 0x12,     /* Set COM pins */
        0x81, 0x7F,     /* Set contrast */
        0xD9, 0xF1,     /* Set precharge period */
        0xDB, 0x40,     /* Set VCOMH deselect level */
        0xA4,           /* Resume display from RAM */
        0xA6,           /* Normal display (not inverted) */
        0xAF             /* Display ON */
    };

    if (!oled_i2c_write_cmd(driver->i2c, init_cmds, sizeof(init_cmds))) {
        return false;
    }

    driver->is_on = true;
    sleep_ms(100);
    
    return true;
}

static bool oled_driver_init_sh1106(oled_driver_t *driver)
{
    uint8_t init_cmds[] = {
        0xAE,           /* Display OFF */
        0xD5, 0x80,     /* Set display clock divide ratio */
        0xA8, 0x3F,     /* Set multiplex ratio (64) */
        0xD3, 0x00,     /* Set display offset */
        0x40,           /* Set start line */
        0x8D, 0x14,     /* Enable charge pump */
        0xA1,           /* Set segment remap */
        0xC8,           /* Set COM output direction */
        0xDA, 0x12,     /* Set COM pins */
        0x81, 0x7F,     /* Set contrast */
        0xD9, 0xF1,     /* Set precharge period */
        0xDB, 0x40,     /* Set VCOMH deselect level */
        0xA4,           /* Resume display from RAM */
        0xA6,           /* Normal display */
        0xAF             /* Display ON */
    };

    if (!oled_i2c_write_cmd(driver->i2c, init_cmds, sizeof(init_cmds))) {
        return false;
    }

    driver->is_on = true;
    sleep_ms(100);
    
    return true;
}

bool oled_driver_set_page(oled_driver_t *driver, uint8_t page)
{
    if (!driver || page >= OLED_PAGES) {
        return false;
    }

    uint8_t cmd = 0xB0 | page;  /* 0xB0 + page number */
    
    return oled_i2c_write_cmd(driver->i2c, &cmd, 1);
}

bool oled_driver_set_column(oled_driver_t *driver, uint8_t col)
{
    if (!driver || col >= OLED_WIDTH) {
        return false;
    }

    uint8_t col_offset = (driver->type == OLED_DISPLAY_SH1106) ? 2 : 0;
    uint8_t adjusted_col = col + col_offset;

    /* Split into lower and upper nibbles */
    uint8_t cmds[2];
    cmds[0] = 0x00 | (adjusted_col & 0x0F);      /* Lower nibble */
    cmds[1] = 0x10 | ((adjusted_col >> 4) & 0x0F); /* Upper nibble */

    return oled_i2c_write_cmd(driver->i2c, cmds, 2);
}

bool oled_driver_write_pixel_data(oled_driver_t *driver, 
                                  const uint8_t *data, size_t len)
{
    if (!driver || !data || len == 0) {
        return false;
    }

    return oled_i2c_write_data(driver->i2c, data, len);
}

bool oled_driver_power_on(oled_driver_t *driver)
{
    if (!driver) {
        return false;
    }

    uint8_t cmd = 0xAF;  /* Display ON */
    driver->is_on = oled_i2c_write_cmd(driver->i2c, &cmd, 1);
    
    return driver->is_on;
}

bool oled_driver_power_off(oled_driver_t *driver)
{
    if (!driver) {
        return false;
    }

    uint8_t cmd = 0xAE;  /* Display OFF */
    driver->is_on = !oled_i2c_write_cmd(driver->i2c, &cmd, 1);
    
    return !driver->is_on;
}

bool oled_driver_set_contrast(oled_driver_t *driver, uint8_t contrast)
{
    if (!driver) {
        return false;
    }

    uint8_t cmds[2];
    cmds[0] = 0x81;           /* Set contrast command */
    cmds[1] = contrast;       /* Contrast value */

    if (oled_i2c_write_cmd(driver->i2c, cmds, 2)) {
        driver->contrast = contrast;
        return true;
    }

    return false;
}

bool oled_driver_set_invert(oled_driver_t *driver, bool invert)
{
    if (!driver) {
        return false;
    }

    uint8_t cmd = invert ? 0xA7 : 0xA6;  /* Inverted or normal */
    
    return oled_i2c_write_cmd(driver->i2c, &cmd, 1);
}
