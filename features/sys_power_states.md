# Feature: System Power States - Suspend, Resume, & Shutdown

> Category: "System Services"
> Prerequisite: `features/hal_spec_power.md`
> Prerequisite: `features/arch_infrastructure.md`

This feature introduces a unified system for managing power states: a low-power "suspend" mode and a full "shutdown" mode. These states will be triggered by short and long presses of the device's main power button or its equivalent, with behavior correctly abstracted for each supported hardware target.

## Scenarios

### Scenario: System Suspend (Short Press)
Given the system is running an application on battery power
When the user performs a short press on the power button
Then the system should enter a low-power suspend mode
And the display should turn off
And the WiFi module should be powered down
And the touch controller should enter a low-power sleep state
And all application and UI rendering processes should be paused

### Scenario: System Resume (Short Press)
Given the system is in the low-power suspend mode
When the user performs a short press on the power button
Then the system should resume normal operation
And the display should turn on
And the WiFi module should be re-initialized and begin connecting
And the touch controller should wake up and report touch events
And all application and UI rendering processes should resume

### Scenario: System Shutdown (Long Press)
Given the system is running an application on battery power
When the user performs a long press (4 seconds) on the power button
Then the system should perform a complete, clean shutdown
And the screen should power off entirely
And all power rails from the PMU should be disabled
And power consumption should drop to near-zero (standby/off state)

### Scenario: System Startup (Long Press)
Given the system is in a complete shutdown state
When the user performs a long press (4 seconds) on the power button
Then the system should perform a full boot sequence
And the LPad mini-logo animation should be displayed
And the system should enter the main application view

---

## Hardware (HIL) Test

**Objective:** Verify that both short-press (suspend/resume) and long-press (shutdown/startup) cycles work correctly on both supported hardware targets.

### Test Steps: Waveshare 1.8" (AXP2101) & LilyGo 1.91" (SY6970)

1.  **Initial State:** Build and flash the firmware for the target board. Ensure the device is running on battery power and the main application UI is visible.
2.  **Test Suspend:**
    *   **Action:** Briefly press and release the main power button (the side button on the Waveshare, the BOOT/GPIO0 button on the LilyGo).
    *   **Expected:** The screen turns off immediately. The device should become unresponsive to touch.
3.  **Test Resume:**
    *   **Action:** Wait 5 seconds. Briefly press and release the same button again.
    *   **Expected:** The screen turns back on, displaying the same UI as before. The WiFi indicator should show it is attempting to reconnect. Touch should be responsive.
4.  **Test Shutdown:**
    *   **Action:** Press and hold the power button for at least 4 seconds.
    *   **Expected:** The screen should turn off. The device is now fully powered down. Subsequent short presses should do nothing.
5.  **Test Startup:**
    *   **Action:** From the shutdown state, press and hold the power button for at least 4 seconds.
    *   **Expected:** The device should boot up, showing the LPad logo animation, and then enter the main application, just as it would if power were freshly applied.

---

## Implementation Notes

This feature requires a new system-level component (`PowerManager`) and a significant expansion of the Power HAL (`hal/power.h`).

### 1. System Component: `PowerManager`

A new singleton or globally accessible class `PowerManager` should be created in `src/system/`.

*   **Responsibilities:**
    *   Initialize and manage the power button event detection via the HAL.
    *   Maintain the current system power state (e.g., `RUNNING`, `SUSPENDED`).
    *   In the main loop, poll for button events from the HAL (`hal_power_button_get_event()`).
    *   Orchestrate the suspend/resume/shutdown sequences by calling the appropriate system services (e.g., `AnimationTicker::pause()`, `hal_network_sleep()`, `hal_display_sleep()`).
*   **Interface (Conceptual):**
    ```cpp
    class PowerManager {
    public:
        void begin();
        void handle(); // Called in main loop
        void suspend();
        void resume();
        void shutdown();
    };
    ```

### 2. HAL Interface Expansion (`hal/power.h`)

The existing `hal/power.h` contract must be extended to abstract power state and button interactions.

*   **Button Event Enum:**
    ```cpp
    typedef enum {
        HAL_POWER_EVENT_NONE,
        HAL_POWER_EVENT_SHORT_PRESS,
        HAL_POWER_EVENT_LONG_PRESS,
    } hal_power_button_event_t;
    ```
*   **New HAL Functions:**
    ```cpp
    // Initializes the power button (GPIO or PMU interrupt)
    void hal_power_button_init(void);

    // Polls for or retrieves a debounced/processed button event
    hal_power_button_event_t hal_power_button_get_event(void);

    // Puts all hardware managed by the PMU/power system to sleep
    void hal_power_suspend(void);

    // Wakes all hardware managed by the PMU/power system
    void hal_power_resume(void);

    // Commands the PMU to cut all power rails
    void hal_power_shutdown(void);
    ```

### 3. Board-Specific HAL Implementations

The core of this feature is handling the different hardware capabilities.

#### For `power_esp32_s3.cpp` (Waveshare / AXP2101)

*   **Button Handling:** Use the AXP2101's built-in Power Enable Key (PEK) feature.
    *   `hal_power_button_init()`:
        *   Enable PEK interrupts: `pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ)`.
        *   Configure long press duration for a 4-second shutdown: `pmu.setPowerKeyPressOffTime(XPOWERS_AXP2101_PRESSOFF_4S)`. The PMU will handle the shutdown automatically.
    *   `hal_power_button_get_event()`:
        *   Read the PMU IRQ status: `pmu.getIrqStatus()`.
        *   Check `pmu.isPekeyShortPressIrq()` and return `HAL_POWER_EVENT_SHORT_PRESS`.
        *   Clear the IRQ status: `pmu.clearIrqStatus()`.
*   **State Management:**
    *   `hal_power_suspend()`: Use `esp_light_sleep_start()` with wakeup source configured for the PMU's IRQ pin. Before sleeping, call `pmu.disable...()` for peripherals not needed.
    *   `hal_power_shutdown()`: Call `pmu.shutdown()`.

#### For `power_tdisplay_s3_plus.cpp` (LilyGo / SY6970)

*   **Button Handling:** Emulate button events using **GPIO 0**. The SY6970 has no PEK feature.
    *   `hal_power_button_init()`: Configure `GPIO 0` as an input with a pull-up resistor.
    *   `hal_power_button_get_event()`:
        *   Implement a software state machine to debounce the GPIO 0 input.
        *   Use `millis()` to measure the press duration.
        *   If press duration is short (< 1 second), return `HAL_POWER_EVENT_SHORT_PRESS` on release.
        *   If press duration exceeds 4 seconds, return `HAL_POWER_EVENT_LONG_PRESS` immediately (don't wait for release).
*   **State Management:**
    *   `hal_power_suspend()`: Use `esp_light_sleep_start()` with `esp_sleep_enable_ext0_wakeup()` configured for GPIO 0.
    *   `hal_power_shutdown()`: Call the PMU's `shutdown()` method (`pmu.shutdown()` which calls `disableBATFET()`). This is a software command to the PMU to cut battery power.

### 4. System Integration

*   The main `loop()` in `main.cpp` must be updated to call `PowerManager::handle()` on every iteration.
*   The `PowerManager` must have access to other system components (`AnimationTicker`, `RenderManager`) to pause/resume them.
*   The chosen suspend mode should be ESP-IDF's **Light Sleep**, as it maintains RAM and allows for fast wakeup from a GPIO or external interrupt.

### [2026-02-16] Waveshare PMU IRQ Routed Through I2C GPIO Expander
The AXP2101 PMU's IRQ output on the Waveshare board is connected to XCA9554 GPIO expander pin 5 (`expander.digitalRead(5)` in vendor examples), NOT to a direct ESP32 GPIO. The vendor SDK sets `PMU_INTERRUPT_PIN = -1`. This means `esp_light_sleep_start()` with ext0/ext1 wakeup cannot be used for the Waveshare board. Instead, `hal_power_suspend()` enters a polling loop (`delay(100)` between PMU IRQ checks via I2C). The LilyGo T-Display S3 Plus CAN use proper `esp_light_sleep_start()` with GPIO 0 ext0 wakeup.

### [2026-02-16] AXP2101 Long-Press Shutdown Is Hardware-Managed
`setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S)` configures the AXP2101 to automatically cut all power rails after a 4-second PEK hold. The software `hal_power_shutdown()` is a fallback that calls `pmu.shutdown()` directly.

### [2026-02-16] T-Display Shutdown Must Use Deep Sleep, Not BATFET Disable
**Problem:** `pmu.shutdown()` calls `disableBATFET()` on the SY6970, cutting the battery-to-system FET. Unlike the AXP2101 which has a hardware PEK (Power Enable Key) to restart, the SY6970 has NO mechanism to re-enable BATFET from a button press. The device is permanently off until USB is connected. On USB, `disableBATFET()` is ignored entirely — the device enters an infinite loop instead of shutting down.
**Fix:** Use `esp_deep_sleep_start()` with GPIO 0 as ext0 wakeup source. Deep sleep draws ~10µA and triggers a full ESP32 reset on button press, producing the expected startup experience (logo animation, WiFi connect). Also reduced software long-press threshold from 4s to 2s — the AXP2101 handles 4s in hardware (PEK), but the T-Display's software detection gives no user feedback during the hold, making 4s feel unresponsive.

### [2026-02-16] Suspend Orchestration Order
Suspend sequence: display sleep → WiFi disconnect → touch sleep → CPU suspend. Resume sequence: CPU resume → power HAL re-seed → display wake → touch wake. WiFi reconnection is NOT automatic on resume — the application layer manages reconnection. This prevents the PowerManager from needing to know WiFi credentials.

### [2026-02-16] Display Sleep Uses MIPI Standard Commands
Both the SH8601 (Waveshare) and RM67162 (LilyGo) support the standard `displayOff()`/`displayOn()` methods from Arduino_GFX. On wake, brightness is restored to 255 (max) to eliminate PWM flicker artifacts. The T-Display's RM67162 also requires re-writing the brightness register (0x51) via the bus after displayOn(). **MIPI requires a 120ms delay after Sleep Out (0x11) before the display controller begins scanning memory.** Without this delay, the Display On command may be ignored, leaving a static (frozen) screen. Added `delay(120)` after `displayOn()` in the Waveshare HAL.

### [2026-02-16] I2C Bus Must Be Explicitly Reinitialized After Light Sleep
On ESP32-S3, `esp_light_sleep_start()` resets the I2C peripheral hardware, but the Arduino Wire library's internal `_started` flag survives in RAM. Any I2C operation using the stale Wire state accesses a de-initialized peripheral, corrupting driver state that persists even after a subsequent `Wire.begin()`. Fix: call `Wire.end()` + `Wire.begin(SDA, SCL)` immediately after waking from light sleep, BEFORE any I2C operations (PMU reads, touch init, etc.). Applied in `hal_power_suspend()` for the T-Display S3 Plus.

### [2026-02-16] Touch Wake Methods Differ By Controller
FT3168 (Waveshare): Wakes from hibernate on any I2C activity — a dummy register read suffices. CST816 (LilyGo): Requires INT pin toggle (drive LOW 50ms, release) to exit auto-sleep, followed by re-disabling auto-sleep (reg 0xFE = 0x01) and re-setting interrupt mode (reg 0xFA = 0x60). **CRITICAL:** The INT pin toggle can only wake CST816 from auto-sleep, NOT from register-initiated deep sleep (0xE5=0x03). Deep sleep requires hardware RST which is not available on the T-Display S3 Plus.

### [2026-02-16] FT3168 Must NOT Be Put to Sleep on Waveshare
The FT3168 on the Waveshare board does not have an exposed RST pin. FocalTech hibernate (0x03) requires hardware RST to wake, and monitor mode (0x01) does not reliably I2C-wake on this board variant. Both cause the touch controller to stop ACKing, flooding the shared I2C bus (PMU + GPIO expander) with `i2cWriteReadNonStop Error -1` and blocking the main loop (static screen). Since the Waveshare uses CPU polling for suspend (not `esp_light_sleep_start`), the FT3168 can simply stay active — its power draw is negligible vs. CPU polling. `hal_touch_sleep()` sets `g_touch_initialized = false` to block reads during suspend, and `hal_touch_wake()` calls `hal_touch_init()` for a clean re-init.

### [2026-02-16] WiFi Driver Must Be Fully Stopped Before Light Sleep
`WiFi.disconnect(true)` alone does NOT fully stop the WiFi driver. The underlying FreeRTOS WiFi task remains running with timer callbacks that reference code stored in flash. During `esp_light_sleep_start()`, flash is memory-mapped out, so these callbacks trigger `InstrFetchProhibited` at `PC=0x00000000` (null pointer / unmapped flash). Fix: call `esp_wifi_stop()` (from `esp_wifi.h`) before entering light sleep. This fully tears down the WiFi driver task and its timers. A 50ms delay after `esp_wifi_stop()` ensures cleanup completes before sleep entry.

### [2026-02-16] CST816 Wake Requires Full Re-Init After Light Sleep
A lightweight wake (INT toggle + register writes) is NOT sufficient for the CST816 on the T-Display after `esp_light_sleep_start()`. The ESP32-S3 I2C peripheral state is lost during light sleep, so `Wire.begin()` must be called again. Fix: `hal_touch_sleep()` sets `g_touch_initialized = false`, and `hal_touch_wake()` calls `hal_touch_init()` which runs the complete proven sequence (Wire.begin, INT pin toggle, auto-sleep disable, interrupt mode config).

### [2026-02-16] T-Display RM67162 Also Requires MIPI 120ms Sleep Out Delay
The 120ms delay after `displayOn()` (MIPI Sleep Out → Display On) was initially only added to the Waveshare SH8601 HAL. The T-Display RM67162 has the same MIPI requirement — without the delay, `Display On` can be ignored after light sleep wake, leaving the screen static (frozen last frame). Added `delay(120)` to `hal_display_wake()` in `display_tdisplay_s3_plus.cpp`.

### [2026-02-16] CST816 Must NOT Use Deep Sleep Command Without RST Pin
**Root Cause:** `hal_touch_sleep()` sent deep sleep command (0xE5=0x03) to the CST816. Without a hardware RST pin on the T-Display S3 Plus, the INT pin toggle cannot wake the chip from register-initiated deep sleep — it can only wake from auto-sleep. After light sleep → wake, the CST816 was unresponsive to I2C, causing `hal_touch_init()` probe failure and dead touch. **Fix:** Remove the deep sleep register write. Instead, re-enable auto-sleep (0xFE=0x00) so the CST816 enters low-power mode naturally. The INT pin toggle in `hal_touch_init()` reliably wakes it from auto-sleep. Same root cause as FT3168 — both boards lack RST pins.
