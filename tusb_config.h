/*
 * TinyUSB Configuration for RP2040
 * Host mode with HID, MSC, and CDC support
 * Copyright (c) 2026
 */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

/* ===== HOST CONFIGURATION ===== */
#define CFG_TUH_ENABLED        1      // Enable TinyUSB host mode

/* ===== PIO USB (RP2040 specific) ===== */
#define CFG_TUH_RPI_PIO_USB    1      // Use PIO-based USB on RP2040

/* ===== USB Class Support ===== */
#define CFG_TUH_HID            1      // Enable HID class (keyboards, mice)
#define CFG_TUH_MSC            1      // Enable Mass Storage Class
#define CFG_TUH_CDC            1      // Enable CDC (serial devices)

/* ===== HID Configuration ===== */
#define CFG_TUH_HID_KEYBOARDMOUSE  1
#define CFG_TUH_HID_GENERIC        1  // Generic HID devices

/* ===== Device Configuration ===== */
#define CFG_TUH_DEVICE_MAX        4   // Support up to 4 USB devices
#define CFG_TUSB_DEBUG           2    // Debug level: 0=off, 1=error, 2=warning, 3=info

/* ===== Memory Configuration ===== */
#define CFG_TUH_ENUM_BUFFER_SIZE   256

/* ===== RTOS Configuration ===== */
// Using bare-metal (no RTOS)
#define CFG_TUSB_OS              OPT_OS_NONE

/* ===== Root Hub Port Configuration ===== */
// RP2040 PIO USB uses specific ports
// Port 0: Internal USB device controller (if using device mode)
// Port 1: PIO-based USB host (GPIO 26=D- / GPIO 27=D+)
#define CFG_TUSB_RHPORT0_MODE    (OPT_MODE_HOST | OPT_MODE_FULL_SPEED)

#ifdef __cplusplus
}
#endif

#endif /* TUSB_CONFIG_H */
