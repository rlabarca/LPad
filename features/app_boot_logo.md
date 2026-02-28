# Feature: Boot Logo App

> Label: "App: Boot Logo"
> Category: "Applications"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/arch_display_pipeline.md
> Prerequisite: features/arch_concurrency.md

[TODO]

## 1. Overview

The BootLogoApp is an AppComponent (Z=5) that plays a one-time boot animation on cold start. It displays the LPad vector logo centered at 75% screen height, waits 2 seconds, then animates it shrinking and easing to the top-right corner over 1.5 seconds. It holds there until a cross-core signal from the WiFi boot task confirms connectivity, then hands off to the main StockTickerApp and shows the MiniLogoComponent. An error screen fallback handles WiFi task failures.

---

## 2. Requirements

### 2.1 State Machine

- States: WAIT -> ANIMATE -> HOLDING -> DONE, with ERROR reachable from any state.
- WAIT: Logo rendered centered at 75% height for 2.0 seconds.
- ANIMATE: Logo shrinks and eases to top-right corner over 1.5 seconds using EaseInOutCubic interpolation.
- HOLDING: Polls `m_bootComplete` volatile flag each frame until set by Core 0 WiFi task.
- DONE: Shows MiniLogoComponent, sets active app to StockTickerApp (single update tick, then self-paused).
- ERROR: Highest priority. If `m_hasError` is true, renders error message once and halts.

### 2.2 Cross-Core Signaling

- `setBootComplete()` is called from Core 0 WiFi task. Sets `volatile bool m_bootComplete`.
- `setErrorMessage(message)` is called from Core 0. Writes the pointer BEFORE setting the `m_hasError` flag to guarantee safe cross-core read ordering.
- No mutex required: single-writer (Core 0) / single-reader (Core 1) pattern with volatile.

### 2.3 Rendering

- First frame: full solid background fill + direct vector logo draw (no flicker risk).
- Animation frames: dirty-rect atomic blit. Union of previous and new logo bounding boxes composited into a temporary Canvas, then DMA blit as single operation.
- Bounding box includes 2px padding on all sides for triangle rasterization rounding.
- Error screen: solid background + centered error message using theme normal font.

### 2.4 Component Properties

- `isOpaque()` returns `true`. `isFullscreen()` returns `true`.
- `handleInput()` always returns `false` (all input passes through).

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

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

#### Scenario: ANIMATE transitions to HOLDING when complete

    Given the state is ANIMATE
    When update is called with cumulative animation time exceeding 1.5 seconds
    Then the state transitions to HOLDING

#### Scenario: HOLDING transitions to DONE on boot complete signal

    Given the state is HOLDING
    When setBootComplete is called from Core 0
    And update is called on Core 1
    Then the state transitions to DONE
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

### Manual Scenarios (Human Verification Required)

#### Scenario: Boot animation plays on device power-on

    Given the device is powered on from cold state
    Then the LPad logo appears centered on screen for 2 seconds
    And it smoothly shrinks and moves to the top-right corner
    And the stock chart appears after WiFi connects
