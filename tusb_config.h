/*
 * TinyUSB Configuration for RP2040 (Raspberry Pi Pico)
 * PlugSafe: BadUSB Detection System
 *
 * Native USB Host mode on port 0 (micro-USB via OTG cable)
 * Copyright (c) 2026
 */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------
// Common Configuration
//--------------------------------------------------------------------

// CFG_TUSB_MCU is defined by the Pico SDK compiler flags
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS           OPT_OS_NONE
#endif

// Debug level: 0=off, 1=error, 2=warning, 3=info
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG        1
#endif

// USB DMA memory section and alignment
#ifndef CFG_TUH_MEM_SECTION
#define CFG_TUH_MEM_SECTION
#endif

#ifndef CFG_TUH_MEM_ALIGN
#define CFG_TUH_MEM_ALIGN     __attribute__ ((aligned(4)))
#endif

//--------------------------------------------------------------------
// Host Configuration
//--------------------------------------------------------------------

// Enable host stack
#define CFG_TUH_ENABLED       1

// RHPort number used for host: port 0 = native USB (micro-USB via OTG)
#ifndef BOARD_TUH_RHPORT
#define BOARD_TUH_RHPORT      0
#endif

// RHPort max operational speed
#ifndef BOARD_TUH_MAX_SPEED
#define BOARD_TUH_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

#define CFG_TUH_MAX_SPEED    BOARD_TUH_MAX_SPEED

//--------------------------------------------------------------------
// Driver Configuration
//--------------------------------------------------------------------

// Size of buffer to hold descriptors and other data used for enumeration
#define CFG_TUH_ENUMERATION_BUFSIZE 256

// Max devices supported (excluding hub): 1 hub typically has 4 ports
#define CFG_TUH_DEVICE_MAX          4

// Hub support: 1 hub so we can detect and warn about hubs on OLED
#define CFG_TUH_HUB                 1

// HID support: combo keyboard+mouse devices can have 3+ HID interfaces
#define CFG_TUH_HID                 (3 * CFG_TUH_DEVICE_MAX)

// We don't need CDC, MSC, or vendor class support
#define CFG_TUH_CDC                 0
#define CFG_TUH_MSC                 0
#define CFG_TUH_VENDOR              0

//------------- HID -------------//
#define CFG_TUH_HID_EPIN_BUFSIZE    64
#define CFG_TUH_HID_EPOUT_BUFSIZE   64

#ifdef __cplusplus
}
#endif

#endif /* TUSB_CONFIG_H */
