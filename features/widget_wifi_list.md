# Feature: WiFi List Widget

> Label: "Widget: WiFi List"
> Category: "UI Framework"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/design_visual_language.md

[TODO]

## 1. Overview

The WiFiListWidget extends ScrollableListWidget to provide a WiFi network selector with visual connection state feedback. It displays configured WiFi entries with color-coded states (normal, active/connected, connecting with blink animation, error/failed) and manages the WiFi connection lifecycle including automatic fallback to the last known good network on failure.

---

## 2. Requirements

### 2.1 Visual States

- **Normal**: text in `normalColor` (cream), no background, no circle.
- **Active/Connected**: text in `highlightColor` (sage), left circle indicator in highlight color.
- **Connecting**: blinking background (`connectingBgColor`, 750ms interval), text in normal color.
- **Error/Failed**: text in `errorColor` (red), no background.

### 2.2 Connection Lifecycle

- Tapping a network entry calls `hal_network_init(ssid, password)` and transitions the item to Connecting state.
- Tapping the already-active network is a no-op.
- `update()` polls `hal_network_get_status()` each frame to detect state transitions.
- On CONNECTED: promote to Active state, update `lastGoodIndex`, fire SSID change callback.
- On ERROR or DISCONNECTED: transition to Error state, mark as `failedIndex`.

### 2.3 Automatic Fallback

- When a connection attempt fails and a `lastGoodIndex` exists (different from the failed network), one automatic fallback attempt is made to the last known good network.
- If the fallback itself fails, no further auto-retry occurs.
- The `isFallback` flag prevents infinite fallback loops.

### 2.4 Blink Animation

- During CONNECTING state, `update()` toggles the item's background on/off every 750ms (BLINK_INTERVAL_MS).
- Blink starts in the ON state when a connection attempt begins.

### 2.5 Refresh

- `refresh()` re-queries HAL network status and resets all item visuals, re-highlighting the currently connected network if any.
- `setEntries(entries, count)` clears and repopulates the list, then checks current HAL connection.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Tap initiates connection attempt

    Given WiFi entries are configured
    And no network is currently connecting
    When the user taps a network entry
    Then hal_network_init is called with that entry's credentials
    And the entry transitions to Connecting state with blinking background

#### Scenario: Tap on active network is ignored

    Given network at index 2 is currently connected (active)
    When the user taps index 2
    Then no connection attempt is made

#### Scenario: Successful connection updates visual state

    Given a network entry is in Connecting state
    When hal_network_get_status returns CONNECTED
    Then the entry transitions to Active state with highlight color and circle
    And the SSID change callback fires

#### Scenario: Failed connection shows error state

    Given a network entry is in Connecting state
    When hal_network_get_status returns ERROR
    Then the entry transitions to Error state with error color text

#### Scenario: Automatic fallback to last good network

    Given network A was previously connected (lastGoodIndex)
    And a connection attempt to network B fails
    When the error is detected in update
    Then an automatic connection to network A is initiated
    And isFallback is set to true

#### Scenario: Fallback failure does not retry

    Given a fallback connection attempt is in progress (isFallback=true)
    When the fallback connection fails
    Then no further automatic reconnection is attempted

#### Scenario: Blink toggles every 750ms during connecting

    Given a network entry is in Connecting state
    When 750ms elapses
    Then the entry's background visibility toggles
    And the next toggle occurs after another 750ms

### Manual Scenarios (Human Verification Required)

#### Scenario: WiFi connection visual feedback on device

    Given the system menu is open on a device with multiple WiFi networks configured
    When the user taps a network name
    Then the entry blinks during connection
    And shows a green highlight with circle indicator when connected
    Or shows red text if the connection fails
