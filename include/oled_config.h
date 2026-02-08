/*
 * OLED Display Configuration
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef OLED_CONFIG_H
#define OLED_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* Display Dimensions */
#define OLED_WIDTH                128
#define OLED_HEIGHT               64
#define OLED_PAGES                8

/* Framebuffer size: WIDTH × (HEIGHT / 8) bytes */
#define OLED_BUFFER_SIZE          1024

/* I2C Configuration */
#define OLED_I2C_ADDRESS_DEFAULT  0x3C
#define OLED_I2C_ADDRESS_ALT      0x3D
#define OLED_I2C_BAUDRATE_DEFAULT 400000

/* I2C Control Bytes */
#define OLED_I2C_CTRL_CMD         0x00
#define OLED_I2C_CTRL_DATA        0x40
#define OLED_I2C_CTRL_CMD_DATA    0x80
#define OLED_I2C_CTRL_CMDS        0xC0

/* Feature Flags */
#define OLED_ENABLE_GRAPHICS      1
#define OLED_ENABLE_TEXT          1
#define OLED_ENABLE_FONTS         1

/* Display Type */
typedef enum {
    OLED_DISPLAY_SSD1306,
    OLED_DISPLAY_SH1106
} oled_display_type_e;

/* Status codes */
typedef enum {
    OLED_OK = 0,
    OLED_ERR_INIT = 1,
    OLED_ERR_I2C = 2,
    OLED_ERR_INVALID = 3,
    OLED_ERR_NOMEM = 4,
    OLED_ERR_HW = 5
} oled_status_e;

#endif /* OLED_CONFIG_H */
