# Release: v0.74 - Battery Status & Metering

> Label: "Release 0.74"
> Category: "RELEASES"
> Prerequisite: features/sys_battery_metering.md

## 1. Introduction
This release introduces system-level power monitoring and a visual battery meter in the top-left of the System Menu. It establishes the architectural pattern for hardware telemetry that does not interfere with UI performance.

## 2. Success Criteria
- [ ] `hal/power.h` contract defined and implemented for `esp32s3` and `tdisplay_s3_plus`.
- [ ] `PowerManager` service correctly polls battery every 2 seconds without UI stutter.
- [ ] `BatteryStatus` data structure correctly updated and accessible.
- [ ] System Menu displays `[NO BATTERY]` when unplugged/no battery.
- [ ] System Menu displays green percentage when charging.
- [ ] System Menu displays red percentage when < 15% and discharging.
- [ ] 10px padding prevents corner clipping on both supported hardware boards.

## 3. Hardware (HIL) Test

### Test 1: Charge State Logic
- **Setup:** Connect LPad to PC via USB.
- **Action:** Open System Menu.
- **Verification:** Battery percentage should be visible in Green in the top-left.

### Test 2: Disconnected State
- **Setup:** (If safe/accessible) Disconnect battery and run on USB.
- **Action:** Open System Menu.
- **Verification:** Top-left should display `[NO BATTERY]` in Khaki.

### Test 3: Visual Padding
- **Setup:** Standard boot.
- **Action:** Open System Menu.
- **Verification:** The battery text on the left and WiFi text on the right MUST NOT be cut off by the rounded screen corners. There should be a visible 10px buffer from the edge.
