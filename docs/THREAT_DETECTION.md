# PlugSafe — Threat Detection

This document explains how PlugSafe detects malicious USB devices, the classification logic, detection thresholds, and design rationale.

## The Problem: BadUSB Attacks

A **BadUSB** (or **Rubber Ducky**) attack uses a device that looks like an ordinary USB peripheral but internally acts as a keyboard. When plugged in, it types pre-programmed keystrokes at superhuman speed — often hundreds of keys per second — to execute commands, download malware, or exfiltrate data. The attack completes in seconds, before the user can react.

These devices are dangerous because:
- The host OS trusts keyboards implicitly (no driver approval needed)
- They enumerate as standard HID devices with legitimate-looking VID/PID values
- The injected keystrokes are indistinguishable from real typing at the OS level
- Antivirus software does not inspect keyboard input

PlugSafe sits between the suspicious device and your computer. It enumerates the device, monitors its behavior, and flags it before you ever plug it into your real machine.

## Threat Levels

PlugSafe classifies every connected USB device into one of three threat levels:

| Level | Display Text | Meaning |
|-------|-------------|---------|
| `THREAT_SAFE` | `SAFE` | Non-HID device (mass storage, audio, etc.) or HID mouse. No keystroke injection risk. |
| `THREAT_POTENTIALLY_UNSAFE` | `CAUTION` | HID keyboard or unknown HID device. Could be legitimate or could be an attack device. Requires monitoring. |
| `THREAT_MALICIOUS` | `MALICIOUS!!!` | HID device confirmed sending keystrokes at >50 keys/second. Almost certainly a scripted injection attack. |

## Classification Pipeline

### Step 1: Initial Classification (on device mount)

When a device is first plugged in, PlugSafe reads its USB device descriptor and performs an initial classification based on device class and HID protocol:

```
Device plugged in
        |
        v
  Is it a HID device?
  |                  |
  No                 Yes
  |                  |
  v                  v
SAFE            What HID protocol?
                |        |        |
           Protocol 1  Proto 2  Proto 0
           (Keyboard)  (Mouse)  (Unknown)
                |        |        |
                v        v        v
           CAUTION     SAFE    CAUTION
```

- **Non-HID devices** (class != 0x03): mass storage drives, audio devices, printers, etc. are always `SAFE`. They cannot inject keystrokes.
- **HID Mouse** (protocol 2): `SAFE`. Mice generate high report rates normally (polling at 125-1000 Hz). They don't type.
- **HID Keyboard** (protocol 1): `CAUTION`. Could be a legitimate keyboard or a Rubber Ducky.
- **Unknown HID** (protocol 0): `CAUTION`. Could be a composite device or custom HID that might inject keystrokes.

### Step 2: Runtime Monitoring (continuous)

For every non-mouse HID device, PlugSafe monitors the rate of incoming HID reports using a **1-second sliding window**:

```
HID report received
        |
        v
  Increment report counter
        |
        v
  Has 1 second elapsed since window start?
  |                                      |
  No                                     Yes
  |                                      |
  (wait for                              v
   more reports)              Calculate rate:
                              current_rate = reports * 1000 / elapsed_ms
                                      |
                                      v
                              Update peak_rate
                              Reset window
                                      |
                                      v
                              Is rate > 50 keys/sec?
                              |                    |
                              No                   Yes
                              |                    |
                              v                    v
                        (stay at current     ESCALATE to MALICIOUS
                         threat level)       (permanent, sticky)
```

### Step 3: Threat Escalation

Threat classification **only escalates, never de-escalates**:

- `SAFE` -> `CAUTION` (if device re-identifies as HID keyboard)
- `CAUTION` -> `MALICIOUS` (if keystroke rate exceeds threshold)
- `MALICIOUS` -> (stays MALICIOUS until device is disconnected)

A `MALICIOUS` device that slows down or stops sending keystrokes remains flagged. This prevents an attack device from hiding its activity after injecting a payload.

## Detection Thresholds

| Constant | Value | Defined In | Purpose |
|----------|-------|-----------|---------|
| `HID_KEYSTROKE_THRESHOLD_HZ` | 50 | `hid_monitor.h`, `threat_analyzer.h` | Reports per second to flag as malicious |
| `KEYSTROKE_RATE_WINDOW_MS` | 1000 | `hid_monitor.h` | Sliding window size for rate calculation |
| `RATE_NORMAL_MAX_HZ` | 30 | `threat_analyzer.h` | Human typing ceiling (informational, not used in logic) |
| `RATE_SUSPICIOUS_MIN_HZ` | 50 | `threat_analyzer.h` | Synonym for the threshold (informational) |

### Why 50 Keys/Second?

- **Normal human typing**: 5-15 keys/second (40-120 WPM)
- **Fast human typing**: 15-25 keys/second (professional typist)
- **Very fast human typing**: 25-30 keys/second (extreme outlier)
- **Rubber Ducky / BadUSB**: 100-1000+ keys/second (scripted injection)

The 50 Hz threshold provides a comfortable margin above the fastest human typists while catching all known attack devices. A Rubber Ducky typically operates at 200-800+ keys/second.

### Why a 1-Second Window?

A 1-second window balances responsiveness with accuracy:
- **Too short** (e.g., 100ms): High variance, susceptible to false positives from normal typing bursts
- **Too long** (e.g., 10s): Slow to detect attacks. A Rubber Ducky payload completes in 1-3 seconds
- **1 second**: Detects an attack within the first second of injection, while being long enough to smooth out normal typing variance

## Hub Detection

PlugSafe detects USB hubs (device class 0x09) and displays a warning page:

```
!!! WARNING !!!
USB HUB DETECTED

Please disconnect
hub and connect
device directly.
```

Hubs are flagged because:
- A malicious hub could hide an attack device behind legitimate devices
- PlugSafe's single-port host mode is designed for direct device analysis
- Hub enumeration of downstream devices adds complexity and attack surface

Hub support is enabled in TinyUSB (`CFG_TUH_HUB = 1`) only for detection purposes, not for enumerating downstream devices.

## What PlugSafe Does NOT Detect

PlugSafe is designed to detect **keystroke injection attacks**. It does not detect:

- **Malicious firmware on mass storage devices** (infected USB drives) — these are a file-level threat, not a HID threat
- **USB killers** (voltage spike devices) — these are electrical attacks, not data attacks
- **Network-based attacks via USB Ethernet adapters** — these don't use HID
- **Slow keystroke injection** (below 50 keys/sec) — a device typing at human speed would evade detection, though such attacks take minutes instead of seconds and are more likely to be noticed by the user
- **Mouse-based attacks** — mouse HID reports are excluded from monitoring to avoid false positives

## Serial Log Output

When a threat is detected, PlugSafe logs detailed information over UART:

```
[THREAT] !!! THREAT ESCALATION for device 1 !!!
[THREAT] Device: Rubber Ducky (VID:0x1234 PID:0x5678)
[THREAT] Rate: 342 reports/sec (threshold: 50)
[THREAT] Level: POTENTIALLY_UNSAFE -> MALICIOUS
[THREAT] WARNING: Device behaving like Rubber Ducky / BadUSB / similar malware!
```

Normal device connections produce informational logs:

```
[USB] Device mounted at address 1
[USB] VID:0x046D PID:0xC31C Class:0x00
[USB] Manufacturer: Logitech
[USB] Product: USB Keyboard
[THREAT] Device 1 classified as POTENTIALLY_UNSAFE (HID Keyboard)
[HID Monitor] Added device 1
```
