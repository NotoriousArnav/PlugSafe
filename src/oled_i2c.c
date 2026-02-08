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

#include "oled_i2c.h"
#include "hardware/gpio.h"

bool oled_i2c_init(oled_i2c_t *i2c)
{
    if (!i2c || !i2c->i2c) {
        return false;
    }

    /* Initialize I2C hardware */
    i2c_init(i2c->i2c, i2c->baudrate);

    /* Set up GPIO pins for I2C */
    gpio_set_function(i2c->sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(i2c->scl_pin, GPIO_FUNC_I2C);

    /* Enable pull-ups on I2C lines */
    gpio_pull_up(i2c->sda_pin);
    gpio_pull_up(i2c->scl_pin);

    return true;
}

bool oled_i2c_write_cmd(oled_i2c_t *i2c, const uint8_t *cmds, size_t len)
{
    if (!i2c || !cmds || len == 0) {
        return false;
    }

    return oled_i2c_write_raw(i2c, OLED_I2C_CTRL_CMD, cmds, len);
}

bool oled_i2c_write_data(oled_i2c_t *i2c, const uint8_t *data, size_t len)
{
    if (!i2c || !data || len == 0) {
        return false;
    }

    return oled_i2c_write_raw(i2c, OLED_I2C_CTRL_DATA, data, len);
}

bool oled_i2c_write_raw(oled_i2c_t *i2c, uint8_t ctrl_byte, 
                        const uint8_t *data, size_t len)
{
    if (!i2c || !data || len == 0) {
        return false;
    }

    /* Build message: control byte + data */
    uint8_t msg[len + 1];
    msg[0] = ctrl_byte;
    for (size_t i = 0; i < len; i++) {
        msg[i + 1] = data[i];
    }

    /* Send via I2C */
    int result = i2c_write_blocking(i2c->i2c, i2c->address, msg, len + 1, false);
    
    return (result == (len + 1));
}
