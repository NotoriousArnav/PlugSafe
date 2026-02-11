/**
 * @file usb_detector.h
 * @brief USB Device Detection Module for Raspberry Pi Pico
 *
 * This module detects USB devices connected to a female USB-A connector
 * via GPIO pin monitoring and provides LED feedback:
 * - Searching Mode: LED blinks every 1 second (0.5s ON, 0.5s OFF)
 * - Device Detected: LED blinks continuously (0.2s ON, 0.2s OFF)
 *
 * Hardware Configuration:
 * - LED (GPIO 25): Built-in Pico LED
 * - VBUS (Pin 40): USB 5V power detection
 * - D+ (GPIO 0): USB data line (Green wire)
 * - D- (GPIO 1): USB data line (White wire)
 * - GND: Common ground
 *
 * @author PlugSafe Project
 * @date 2026
 */

#ifndef USB_DETECTOR_H
#define USB_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * GPIO PIN DEFINITIONS
 * ============================================================================ */

/** @brief Built-in LED on Raspberry Pi Pico */
#define USB_DETECTOR_LED_PIN 25

/** @brief USB VBUS (5V) input pin */
#define USB_DETECTOR_VBUS_PIN 40

/** @brief USB D+ signal pin (Green wire from female USB-A connector) */
#define USB_DETECTOR_DP_PIN 0

/** @brief USB D- signal pin (White wire from female USB-A connector) */
#define USB_DETECTOR_DM_PIN 1

/* ============================================================================
 * TIMING CONSTANTS (in milliseconds)
 * ============================================================================ */

/** @brief Search mode blink period: 1000ms total (500ms ON, 500ms OFF) */
#define USB_DETECTOR_SEARCH_BLINK_PERIOD_MS 1000

/** @brief Device detected blink period: 400ms total (200ms ON, 200ms OFF) */
#define USB_DETECTOR_DEVICE_BLINK_PERIOD_MS 400

/** @brief VBUS debounce delay to prevent false positives (50ms) */
#define USB_DETECTOR_VBUS_DEBOUNCE_MS 50

/** @brief Main update loop recommended interval (20ms) */
#define USB_DETECTOR_UPDATE_INTERVAL_MS 20

/* ============================================================================
 * STATE ENUMERATION
 * ============================================================================ */

/**
 * @enum usb_detector_state_t
 * @brief States of the USB device detector
 */
typedef enum {
    USB_DETECTOR_STATE_SEARCHING = 0,  /**< No device detected, searching for connection */
    USB_DETECTOR_STATE_DETECTED = 1    /**< USB device detected on port */
} usb_detector_state_t;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

/**
 * @brief Initialize USB detector hardware and state
 *
 * Performs the following initialization:
 * - Configures LED GPIO (GP25) as output
 * - Configures D+ (GP0) and D- (GP1) as inputs with pull-down
 * - Initializes VBUS pin for analog reading (if using ADC)
 * - Sets initial state to SEARCHING
 * - Initializes debounce counter and LED timer
 *
 * @note Must be called before any other USB detector functions
 * @note Should be called early in main() after stdio_init_all()
 */
void usb_detector_init(void);

/**
 * @brief Update USB detector state and LED control
 *
 * This is the main state machine function that should be called
 * periodically (every 10-50ms recommended).
 *
 * The function performs:
 * - Reads VBUS and D+/D- pin states
 * - Applies debouncing to prevent false transitions
 * - Updates internal state (SEARCHING vs DETECTED)
 * - Manages LED blinking pattern based on current state
 *
 * @note Call this function in the main event loop
 * @note Typical usage: while(1) { usb_detector_update(); sleep_ms(20); }
 * @see usb_detector_get_state() to check current state
 * @see usb_detector_is_device_connected() for quick connection check
 */
void usb_detector_update(void);

/**
 * @brief Get the current detector state
 *
 * @return usb_detector_state_t Current state (SEARCHING or DETECTED)
 * @see usb_detector_is_device_connected() for boolean check
 */
usb_detector_state_t usb_detector_get_state(void);

/**
 * @brief Check if a USB device is currently connected
 *
 * Convenience function that returns true if state is DETECTED,
 * false if state is SEARCHING.
 *
 * @return true if USB device is connected, false otherwise
 * @see usb_detector_get_state() for full state information
 */
bool usb_detector_is_device_connected(void);

/**
 * @brief Get the time in milliseconds since last state change
 *
 * Useful for debugging and understanding connection stability.
 * Resets when state transitions between SEARCHING and DETECTED.
 *
 * @return uint32_t Milliseconds since last state transition
 */
uint32_t usb_detector_get_state_duration_ms(void);

/**
 * @brief Manually control LED for testing/debugging
 *
 * This function allows direct LED control for testing purposes.
 * Useful for verifying LED hardware and GPIO wiring.
 *
 * @param state 1 = LED ON, 0 = LED OFF
 * @note Calling this disables automatic LED blinking until next usb_detector_init()
 */
void usb_detector_set_led(uint8_t state);

#endif /* USB_DETECTOR_H */
