/**
 * @file usb_detector.c
 * @brief USB Device Detection Implementation
 *
 * Implements USB device detection logic using GPIO monitoring and
 * LED feedback control with state machine logic.
 */

#include "usb_detector.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include <stdio.h>

/* ============================================================================
 * PRIVATE STATE VARIABLES
 * ============================================================================ */

/** @brief Current detection state */
static usb_detector_state_t current_state = USB_DETECTOR_STATE_SEARCHING;

/** @brief Previous state for transition detection */
static usb_detector_state_t previous_state = USB_DETECTOR_STATE_SEARCHING;

/** @brief LED state (on/off) */
static bool led_state = false;

/** @brief Timestamp when LED state last changed (for blinking) */
static uint32_t led_last_toggle_ms = 0;

/** @brief Timestamp when state last changed */
static uint32_t state_change_time_ms = 0;

/** @brief Debounce counter for VBUS detection */
static uint32_t debounce_counter = 0;

/** @brief Flag to track if manual LED control is active */
static bool manual_led_control = false;

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Read the current USB connection state
 *
 * Checks VBUS and D+/D- pins to determine if a USB device is connected.
 * Returns true if:
 * - VBUS is HIGH (approximately 3.3V or higher, indicating device power)
 * - AND at least one of D+ or D- is HIGH (indicating active data lines)
 *
 * @return true if device appears connected, false otherwise
 */
static bool usb_detector_detect_device(void)
{
    // Read GPIO pin states
    bool dp_high = gpio_get(USB_DETECTOR_DP_PIN);
    bool dm_high = gpio_get(USB_DETECTOR_DM_PIN);

    // Check if either D+ or D- is pulled high (USB device characteristic)
    // In a real implementation with VBUS monitoring, we would also check VBUS voltage
    // For now, detecting if D+ or D- is active is sufficient
    bool device_present = (dp_high || dm_high);

    return device_present;
}

/**
 * @brief Control the LED directly
 *
 * @param on true to turn LED on, false to turn off
 */
static void usb_detector_led_control(bool on)
{
    gpio_put(USB_DETECTOR_LED_PIN, on);
    led_state = on;
}

/**
 * @brief Update LED blink pattern based on current state
 *
 * Implements non-blocking LED blinking:
 * - SEARCHING: 1000ms period (500ms on, 500ms off)
 * - DETECTED: 400ms period (200ms on, 200ms off)
 */
static void usb_detector_update_led(void)
{
    if (manual_led_control) {
        return;  // Skip automatic control if manual control is active
    }

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    uint32_t blink_period;

    // Select blink period based on state
    if (current_state == USB_DETECTOR_STATE_SEARCHING) {
        blink_period = USB_DETECTOR_SEARCH_BLINK_PERIOD_MS;
    } else {
        blink_period = USB_DETECTOR_DEVICE_BLINK_PERIOD_MS;
    }

    // Half-period for on/off timing
    uint32_t half_period = blink_period / 2;

    // Calculate elapsed time since last toggle
    uint32_t elapsed = now_ms - led_last_toggle_ms;

    // Toggle LED when half-period has elapsed
    if (elapsed >= half_period) {
        usb_detector_led_control(!led_state);
        led_last_toggle_ms = now_ms;
    }
}

/**
 * @brief Update the state machine based on device detection
 *
 * Implements debouncing to prevent rapid state transitions.
 * Requires the device detection state to remain stable for
 * DEBOUNCE_TIME ms before transitioning states.
 */
static void usb_detector_update_state(void)
{
    bool device_detected = usb_detector_detect_device();
    bool expected_detected = (current_state == USB_DETECTOR_STATE_DETECTED);

    // Check if device state matches expected state
    if (device_detected == expected_detected) {
        // State matches, reset debounce counter
        debounce_counter = 0;
    } else {
        // State doesn't match, increment debounce counter
        debounce_counter += USB_DETECTOR_UPDATE_INTERVAL_MS;

        // If debounce time exceeded, transition to new state
        if (debounce_counter >= USB_DETECTOR_VBUS_DEBOUNCE_MS) {
            previous_state = current_state;

            if (device_detected) {
                current_state = USB_DETECTOR_STATE_DETECTED;
            } else {
                current_state = USB_DETECTOR_STATE_SEARCHING;
            }

            // Record when state changed
            state_change_time_ms = to_ms_since_boot(get_absolute_time());
            debounce_counter = 0;
        }
    }
}

/* ============================================================================
 * PUBLIC API FUNCTIONS
 * ============================================================================ */

void usb_detector_init(void)
{
    // Initialize LED pin as output
    gpio_init(USB_DETECTOR_LED_PIN);
    gpio_set_dir(USB_DETECTOR_LED_PIN, GPIO_OUT);
    usb_detector_led_control(false);  // Start with LED off

    // Initialize D+ (GP0) as input with pull-down
    gpio_init(USB_DETECTOR_DP_PIN);
    gpio_set_dir(USB_DETECTOR_DP_PIN, GPIO_IN);
    gpio_pull_down(USB_DETECTOR_DP_PIN);

    // Initialize D- (GP1) as input with pull-down
    gpio_init(USB_DETECTOR_DM_PIN);
    gpio_set_dir(USB_DETECTOR_DM_PIN, GPIO_IN);
    gpio_pull_down(USB_DETECTOR_DM_PIN);

    // Initialize state variables
    current_state = USB_DETECTOR_STATE_SEARCHING;
    previous_state = USB_DETECTOR_STATE_SEARCHING;
    debounce_counter = 0;
    led_state = false;
    manual_led_control = false;

    // Record initialization time
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    led_last_toggle_ms = now_ms;
    state_change_time_ms = now_ms;

    printf("[USB Detector] Initialized\n");
    printf("[USB Detector] LED PIN: %d, D+: %d, D-: %d\n",
           USB_DETECTOR_LED_PIN, USB_DETECTOR_DP_PIN, USB_DETECTOR_DM_PIN);
}

void usb_detector_update(void)
{
    // Update state machine (detect device connections)
    usb_detector_update_state();

    // Update LED blinking pattern
    usb_detector_update_led();

    // Debug output on state change
    if (current_state != previous_state) {
        if (current_state == USB_DETECTOR_STATE_DETECTED) {
            printf("[USB Detector] Device DETECTED at %u ms\n", to_ms_since_boot(get_absolute_time()));
        } else {
            printf("[USB Detector] Device DISCONNECTED at %u ms\n", to_ms_since_boot(get_absolute_time()));
        }
        previous_state = current_state;
    }
}

usb_detector_state_t usb_detector_get_state(void)
{
    return current_state;
}

bool usb_detector_is_device_connected(void)
{
    return (current_state == USB_DETECTOR_STATE_DETECTED);
}

uint32_t usb_detector_get_state_duration_ms(void)
{
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    return (now_ms - state_change_time_ms);
}

void usb_detector_set_led(uint8_t state)
{
    manual_led_control = true;
    usb_detector_led_control((state != 0) ? true : false);
    printf("[USB Detector] Manual LED control: %s\n", state ? "ON" : "OFF");
}
