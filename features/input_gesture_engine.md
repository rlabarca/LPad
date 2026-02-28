# Feature: Touch Gesture Engine

> Label: "Input: Gesture Engine"
> Category: "Input"
> Prerequisite: features/arch_hal_contract.md
> Prerequisite: features/arch_component_model.md

[TODO]

## 1. Overview

The TouchGestureEngine is a state machine that transforms raw touch point data into semantic gesture events: TAP, HOLD, HOLD_DRAG, SWIPE, and EDGE_DRAG. It uses axis-aware distance thresholds (horizontal gestures measured against screen width, vertical against height), supports configurable edge zones for hardware-specific touch panels, and outputs both pixel and percentage coordinates. The engine is called once per frame from the main loop.

---

## 2. Requirements

### 2.1 Gesture Types

- **TAP**: Finger down + up with < 5% movement and < 500ms duration.
- **HOLD**: Finger down for >= 500ms with < 5% movement. Emitted once at threshold crossing.
- **HOLD_DRAG**: Movement > 5% after a HOLD was already emitted. Emitted every frame during drag.
- **SWIPE**: Finger up with >= 8% movement (axis-aware) from non-edge start position.
- **EDGE_DRAG**: Finger up with >= 30% movement (axis-aware) from an edge-adjacent start position.

### 2.2 State Machine

- IDLE -> PRESSED (finger down, records start position).
- PRESSED -> HOLD_DETECTED (500ms elapsed, < 5% movement).
- PRESSED -> DRAGGING (> 5% movement).
- PRESSED -> IDLE (finger up, < 5% movement = TAP).
- HOLD_DETECTED -> DRAGGING (> 5% movement).
- HOLD_DETECTED -> IDLE (finger up, no event).
- DRAGGING -> IDLE (finger up, evaluates SWIPE or EDGE_DRAG thresholds).

### 2.3 Thresholds

- HOLD_THRESHOLD_MS: 500ms.
- MOVEMENT_THRESHOLD_PERCENT: 5% (of max screen dimension).
- SWIPE_DISTANCE_PERCENT: 8% (axis-aware: horizontal uses width, vertical uses height).
- EDGE_THRESHOLD_PERCENT: 30% (zone from edge).
- EDGE_SWIPE_DISTANCE_PERCENT: 30% (3.75x the center swipe threshold).

### 2.4 Edge Detection

- Default: edge zone is 30% from each screen edge.
- `setEdgeZones(left, right, top, bottom)` overrides with fixed pixel thresholds for hardware-specific calibration.
- EDGE_DRAG direction reports the **originating edge** (not travel direction): start at top = UP, start at bottom = DOWN, etc.

### 2.5 Coordinate Output

- `x_percent = x_px / screen_width`, `y_percent = y_px / screen_height`, clamped to [0.0, 1.0].

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Quick tap produces TAP event

    Given the engine is in IDLE state
    When a finger touches at (50%, 50%) for 200ms with no movement and lifts
    Then a TOUCH_TAP event is produced at (50%, 50%)

#### Scenario: Hold for 500ms produces HOLD event

    Given the engine is in IDLE state
    When a finger touches and remains stationary for 500ms
    Then a TOUCH_HOLD event is produced

#### Scenario: Movement during hold produces HOLD_DRAG

    Given a HOLD event has been emitted
    When the finger moves more than 5% of screen dimension
    Then TOUCH_HOLD_DRAG events are produced each frame

#### Scenario: Fast swipe produces SWIPE event

    Given a finger touches in the center area
    When it moves 10% horizontally and lifts in under 500ms
    Then a TOUCH_SWIPE event is produced with LEFT or RIGHT direction

#### Scenario: Edge drag from top produces EDGE_DRAG UP

    Given a finger touches within the top 30% of the screen
    When it moves 30% downward and lifts
    Then a TOUCH_EDGE_DRAG event is produced with direction TOUCH_DIR_UP

#### Scenario: Small movement on release does not produce swipe

    Given a finger touches and moves only 5% before lifting
    When under 500ms has elapsed
    Then a TOUCH_TAP event is produced (not a SWIPE)

#### Scenario: Axis-aware thresholds use correct dimension

    Given the screen is 536 wide and 240 tall
    When a horizontal swipe of 8% of 536 pixels occurs
    Then SWIPE is detected using the width-based threshold

#### Scenario: Custom edge zones override defaults

    Given setEdgeZones(40, 430, 36, 204) is called
    When a touch starts at x=30 (within left edge zone)
    Then it is classified as starting from the left edge

### Manual Scenarios (Human Verification Required)

#### Scenario: Gesture recognition on device

    Given the firmware is running on a touchscreen device
    When the user performs taps, swipes, and edge drags
    Then each gesture is correctly recognized and routed to the appropriate UI component
