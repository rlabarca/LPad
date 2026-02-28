# Feature: Boot Logo App

> Label: "App: Boot Logo"
> Category: "Applications"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/arch_display_pipeline.md
> Prerequisite: features/arch_concurrency.md
> Prerequisite: features/hal_network.md
> Prerequisite: features/design_visual_language.md

[TODO]

## 1. Overview

The BootLogoApp is an AppComponent (Z=5) that plays a one-time boot animation on cold start and provides visual feedback about WiFi connection progress. It displays the LPad vector logo centered at 75% screen height, waits 2 seconds, then animates it shrinking and easing to the top-right corner over 1.5 seconds. Upon animation completion, the app enters a CONNECTING state that displays themed status text while polling the Network HAL for visual feedback. Once the cross-core signal from the WiFi boot task confirms connectivity, the app briefly displays the connected SSID, then hands off to the main StockTickerApp and shows the MiniLogoComponent. An error screen fallback handles WiFi task failures.

---

## 2. Requirements

### 2.1 State Machine

- States: WAIT -> ANIMATE -> CONNECTING -> DONE, with ERROR reachable from any state.
- WAIT: Logo rendered centered at 75% height for 2.0 seconds.
- ANIMATE: Logo shrinks and eases to top-right corner over 1.5 seconds using EaseInOutCubic interpolation.
- CONNECTING: Logo stationary in top-right corner. Polls `hal_network_get_status()` each frame for visual feedback. Displays centered status text based on network status:
  - When `hal_network_get_status()` returns CONNECTING or DISCONNECTED: shows "Connecting" with an animated ellipsis (0–3 dots, cycling at ~400ms per step).
  - When `m_bootComplete` flag is set: shows "Connected to \<SSID\>" (SSID retrieved via `hal_network_get_ssid()`) held for 1.5 seconds, then transitions to DONE.
  - Early completion: if `m_bootComplete` is already set when entering CONNECTING (WiFi connected during WAIT or ANIMATE), skip the ellipsis and show "Connected to \<SSID\>" immediately.
- DONE: Shows MiniLogoComponent, sets active app to StockTickerApp (single update tick, then self-paused).
- ERROR: Highest priority. If `m_hasError` is true, renders error message once and halts.

### 2.2 Cross-Core Signaling

- `setBootComplete()` is called from Core 0 WiFi task. Sets `volatile bool m_bootComplete`.
- `setErrorMessage(message)` is called from Core 0. Writes the pointer BEFORE setting the `m_hasError` flag to guarantee safe cross-core read ordering.
- No mutex required: single-writer (Core 0) / single-reader (Core 1) pattern with volatile.
- HAL polling (`hal_network_get_status()`, `hal_network_get_ssid()`) is for visual feedback only. Definitive state transitions (CONNECTING→DONE, any→ERROR) are driven exclusively by the `m_bootComplete` and `m_hasError` volatile flags set from Core 0.

### 2.3 Rendering

- First frame: full solid background fill + direct vector logo draw (no flicker risk).
- Animation frames: dirty-rect atomic blit. Union of previous and new logo bounding boxes composited into a temporary Canvas, then DMA blit as single operation.
- Bounding box includes 2px padding on all sides for triangle rasterization rounding.
- Error screen: solid background + centered error message using theme normal font.
- Status text (CONNECTING state):
  - Font: `theme->fonts.normal` (12pt proportional sans-serif).
  - "Connecting..." color: `theme->colors.text_main` (Khaki).
  - "Connected to \<SSID\>" color: `theme->colors.text_main` (Khaki).
  - "No Network Found" (on error) color: `theme->colors.text_error` (red).
  - Text vertically centered on screen.
  - Horizontal positioning: the x-origin is pre-computed by measuring the max-width string ("Connecting...") and centering that bounding box. All ellipsis variants ("Connecting", "Connecting.", "Connecting..", "Connecting...") are left-aligned from that same x-origin. The base text never shifts; dots only appear/disappear on the right edge. This avoids re-centering jitter with the proportional font.
  - Text updates MUST use dirty-rect blitting per `arch_display_pipeline.md` Tear Prevention: the previous frame's text bounding box is tracked, the union of old and new bounds is composited into a temporary canvas, and the result is DMA-blitted as a single atomic operation. Direct draw-erase-redraw on the live display is prohibited.
  - The dirty-rect bounding box uses the max-width measurement ("Connecting...") as a fixed width, ensuring dot removal never leaves residual pixels.
  - Before DONE transition: text area erased with solid `theme->colors.background` fill to leave a clean screen for the next app.

### 2.4 Component Properties

- `isOpaque()` returns `true`. `isFullscreen()` returns `true`.
- `handleInput()` always returns `false` (all input passes through).

### 2.5 Network HAL Dependency

- During CONNECTING state, each frame calls `hal_network_get_status()` to determine what status text to display.
- Status-to-visual mapping:
  - `HAL_NETWORK_STATUS_CONNECTING` or `HAL_NETWORK_STATUS_DISCONNECTED` → "Connecting" + animated ellipsis.
  - `HAL_NETWORK_STATUS_CONNECTED` → "Connected to \<SSID\>" (SSID retrieved via `hal_network_get_ssid()`).
  - `HAL_NETWORK_STATUS_ERROR` → "No Network Found" in `text_error` color.
- `hal_network_get_ssid()` is called only after `m_bootComplete` is set or `hal_network_get_status()` returns CONNECTED, ensuring the SSID string is stable.
- Cross-core safety: `hal_network_get_status()` and `hal_network_get_ssid()` are called from Core 1 (render loop). The Network HAL state is written on Core 0. These functions return simple values (enum, const char*) and are safe under the single-writer/single-reader volatile pattern documented in Section 2.2.

---

## 3. Scenarios

### Automated Scenarios

#### Scenario: Begin rejects null pointers

    Given a BootLogoApp instance
    When begin is called with a null display or null nextApp
    Then it returns false

#### Scenario: State starts at WAIT

    Given begin has succeeded
    When onRun is called
    Then the state is WAIT

#### Scenario: WAIT transitions to ANIMATE after 2 seconds

    Given the state is WAIT
    When update is called with cumulative dt exceeding 2.0 seconds
    Then the state transitions to ANIMATE

#### Scenario: ANIMATE transitions to CONNECTING when complete

    Given the state is ANIMATE
    When update is called with cumulative animation time exceeding 1.5 seconds
    Then the state transitions to CONNECTING

#### Scenario: CONNECTING transitions to DONE after connected hold time

    Given the state is CONNECTING
    And hal_network_get_status returns CONNECTING
    When setBootComplete is called from Core 0
    And update is called on Core 1
    Then the status text changes to "Connected to <SSID>"
    And after 1.5 seconds the state transitions to DONE
    And the active app is set to the next app

#### Scenario: Error state takes priority over all other states

    Given the state is ANIMATE
    When setErrorMessage is called with "WiFi Failed"
    And update is called
    Then the state transitions to ERROR

#### Scenario: Error message renders once

    Given the state is ERROR
    When render is called twice
    Then the error message is drawn only on the first call

#### Scenario: Ellipsis animation cycles during CONNECTING

    Given the state is CONNECTING
    And hal_network_get_status returns CONNECTING
    When update is called repeatedly at ~400ms intervals
    Then the displayed text cycles through "Connecting", "Connecting.", "Connecting..", "Connecting..."

#### Scenario: Ellipsis text does not shift horizontally during animation

    Given the state is CONNECTING
    And the ellipsis animation is cycling
    When the dot count changes between frames
    Then the "Connecting" base text x-position remains constant
    And only dots appear or disappear on the right edge

#### Scenario: Status text uses dirty-rect blitting

    Given the state is CONNECTING
    When the status text changes between frames
    Then the previous text bounding box and new text bounding box are unioned
    And the union region is composited into a temporary canvas
    And the canvas is DMA-blitted as a single atomic operation

#### Scenario: Connected SSID displayed on boot complete

    Given the state is CONNECTING
    And hal_network_get_ssid returns "MyNetwork"
    When setBootComplete is called
    And update is called
    Then the status text shows "Connected to MyNetwork"

#### Scenario: Connected message holds 1.5 seconds before DONE

    Given the state is CONNECTING
    And setBootComplete has been called
    When update is called with cumulative dt less than 1.5 seconds after boot complete
    Then the state remains CONNECTING
    When update is called after 1.5 seconds total
    Then the state transitions to DONE

#### Scenario: Early WiFi completion skips ellipsis

    Given the state is ANIMATE
    And setBootComplete is called during ANIMATE
    When the state transitions to CONNECTING
    Then the status text immediately shows "Connected to <SSID>"
    And the ellipsis animation is not displayed

#### Scenario: Network error displays error text

    Given the state is CONNECTING
    And hal_network_get_status returns HAL_NETWORK_STATUS_ERROR
    When update is called
    Then the status text shows "No Network Found" in text_error color

#### Scenario: Status text erased before DONE transition

    Given the state is CONNECTING
    And the connected hold time has elapsed
    When the state transitions to DONE
    Then the text area is erased with a solid background fill before the next app renders

### Manual Scenarios (Human Verification Required)

#### Scenario: Boot animation plays with WiFi status feedback

    Given the device is powered on from cold state with valid WiFi credentials
    Then the LPad logo appears centered on screen for 2 seconds
    And it smoothly shrinks and moves to the top-right corner
    And "Connecting..." text with animated dots appears on screen
    And the base "Connecting" text stays fixed while dots animate on the right
    And no flickering or tearing is visible during any text transition
    And once WiFi connects, the text changes to "Connected to <SSID>"
    And after a brief hold the stock chart appears

#### Scenario: Boot animation shows error on WiFi failure

    Given the device is powered on from cold state with invalid WiFi credentials
    Then the LPad logo appears centered on screen for 2 seconds
    And it smoothly shrinks and moves to the top-right corner
    And "Connecting..." text appears centered on screen
    And after WiFi fails, "No Network Found" appears in red text
