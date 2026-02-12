# Feature: Touch Gesture Engine

> Label: "Touch Gesture Engine"
> Category: "Application Layer"
> Prerequisite: features/hal_spec_touch.md

## 1. Introduction
The Touch Gesture Engine interprets raw data from the HAL (`hal_touch_read`) and converts it into high-level semantic events (Tap, Swipe, Hold, Drag). It also provides a coordinate system that supports both absolute pixels and relative percentages, ensuring UI components can handle different aspect ratios.

## 2. Event Definitions

The engine must detect and report the following high-level semantic events:

### A. Tap
- **Condition:** A quick finger press and release.
- **Constraint:** The finger must remain within a small movement threshold of the original touch point to avoid being classified as a drag.
- **Outcome:** Fires on release.

### B. Hold
- **Condition:** A finger press maintained for a specific duration.
- **Constraint:** The finger must remain within a small movement threshold during the hold period.
- **Outcome:** Fires once when the duration threshold is reached.

### C. Hold And Drag
- **Condition:** A "Hold" event has occurred, and subsequently, the finger moves significantly from the original hold location.
- **Outcome:** Fires continuously as the touch position changes while the finger remains pressed.

### D. Swipe (Center)
- **Condition:** A fast directional movement starting from the center region of the screen.
- **Start Constraint:** Must NOT start near screen edges (to distinguish from edge drags).
- **Direction:** The primary axis of movement defines the swipe direction (UP, DOWN, LEFT, RIGHT).
- **Outcome:** Fires on release if the movement distance exceeds the swipe threshold.

### E. Edge Drag (Left, Right, Top, Bottom)
- **Condition:** A touch that starts near one of the screen's edges and moves towards the interior.
- **Start Constraint:** The touch must originate within a narrow zone along the screen's boundaries.
- **Direction:** The originating edge defines the event type (e.g., "Left Edge Drag" if starting from the left-most area).
- **Outcome:** Fires as the movement progresses or upon completion (implementation choice), distinct from standard interior interactions.

## 3. Data Interface

### 3.1 Event Structure
The engine must publish events containing:
- **Type:** `TOUCH_TAP`, `TOUCH_HOLD`, `TOUCH_HOLD_DRAG`, `TOUCH_SWIPE`, `TOUCH_EDGE_DRAG`
- **Direction:** `UP`, `DOWN`, `LEFT`, `RIGHT`, `NONE` (for Swipes and Edge Drags)
- **Position (Absolute):** `x_px`, `y_px`
- **Position (Relative):** `x_percent` (0.0-1.0), `y_percent` (0.0-1.0)

## 4. Scenarios

### Scenario: Detecting a Tap
- Given the user presses at a location
- And releases quickly without significant movement
- Then a `TOUCH_TAP` event is generated

### Scenario: Detecting a Hold
- Given the user presses at a location
- And remains still for the required hold duration
- Then a `TOUCH_HOLD` event is generated

### Scenario: Detecting a Center Swipe
- Given the user presses in the center region of the screen
- And moves their finger in a clear direction (e.g., upward)
- Then a `TOUCH_SWIPE` event with direction `UP` is generated

### Scenario: Detecting an Edge Drag
- Given the user presses near the left edge of the screen
- And moves their finger towards the center
- Then a `TOUCH_EDGE_DRAG` event with direction `LEFT` (representing the origin) is generated

### Scenario: Relative Coordinates
- Given the screen has a known width and height
- And a touch event occurs at the exact center
- Then the reported `x_percent` and `y_percent` should be 0.50
