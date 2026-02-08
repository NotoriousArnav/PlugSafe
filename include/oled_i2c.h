/*
 * OLED I2C Abstraction Layer
 * Hardware-independent I2C communication for OLED displays
 * Copyright (c) 2026
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef OLED_I2C_H
#define OLED_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware/i2c.h"

/* I2C Control Bytes for OLED Displays */
#define OLED_I2C_CTRL_CMD   0x00    /* Control byte for command data */
#define OLED_I2C_CTRL_DATA  0x40    /* Control byte for pixel data */

/* I2C configuration and state */
typedef struct {
    i2c_inst_t *i2c;        /* Pico I2C instance (i2c0 or i2c1) */
    uint sda_pin;           /* GPIO pin for SDA */
    uint scl_pin;           /* GPIO pin for SCL */
    uint baudrate;          /* Baud rate in Hz */
    uint8_t address;        /* 7-bit I2C address */
} oled_i2c_t;

/* Initialize I2C hardware */
bool oled_i2c_init(oled_i2c_t *i2c);

/* Send command byte(s) */
bool oled_i2c_write_cmd(oled_i2c_t *i2c, const uint8_t *cmds, size_t len);

/* Send pixel data */
bool oled_i2c_write_data(oled_i2c_t *i2c, const uint8_t *data, size_t len);

/* Low-level raw write with control byte */
bool oled_i2c_write_raw(oled_i2c_t *i2c, uint8_t ctrl_byte, 
                        const uint8_t *data, size_t len);

#endif /* OLED_I2C_H */
