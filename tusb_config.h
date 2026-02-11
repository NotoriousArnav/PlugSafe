/*
 * TinyUSB Configuration for RP2040
 * Copyright (c) 2026
 */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

/* ===== FUNDAMENTAL CONFIGURATION ===== */

/* Disable TinyUSB - full PIO USB support requires external pio_usb library */
/* For now, we use GPIO-based detection with TinyUSB disabled */
#define CFG_TUH_ENABLED                0

/* Operating System Selection (bare metal) */
#define CFG_TUSB_OS                    OPT_OS_NONE

/* ===== DEBUG LEVEL ===== */
/* 0 = off, 1 = error, 2 = warning, 3 = info, 4 = debug */
#define CFG_TUSB_DEBUG                 2

#ifdef __cplusplus
}
#endif

#endif /* TUSB_CONFIG_H */
