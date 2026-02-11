# Feature: Touch Gesture Engine

> Label: "Touch Gesture Engine"
> Category: "Application Layer"
> Prerequisite: features/hal_spec_touch.md

## 1. Introduction
The Touch Gesture Engine interprets raw data from the HAL (`hal_touch_read`) and converts it into high-level semantic events (Tap, Swipe, Hold, Drag). It also provides a coordinate system that supports both absolute pixels and relative percentages, ensuring UI components can handle different aspect ratios.

## 2. Event Definitions

The engine must detect and report the following events:

### A. Tap
- **Condition:** Finger press and release < 1.0 second.
- **Constraint:** Finger must not move more than 10% of screen max dimension from the original touch point.
- **Outcome:** Fires on release.

### B. Hold
- **Condition:** Finger press > 1.0 second.
- **Constraint:** Finger must not move more than 10% of screen max dimension.
- **Outcome:** Fires continuously or once after threshold (design choice: fires once when threshold reached).

### C. Hold And Drag
- **Condition:** A "Hold" event has already occurred, AND the finger moves > 10% from the original hold location.
- **Outcome:** Fires continuously as touch position changes.
- **Note:** This is the *only* event that continues to report position updates during the drag phase after the hold.

### D. Swipe (Left, Right, Up, Down)
- **Condition:** Fast movement (velocity or distance check).
- **Start Constraint:** Must START 20% or more from any edge (i.e., strictly inside the central 60% area).
- **Distance Constraint:** Must cover > 40% of the screen dimension in the primary direction.
- **Direction:** Determined by the axis with the largest delta.
- **Outcome:** Fires on release.

### E. Edge Drag (Left, Right, Top, Bottom)
- **Condition:** Movement similar to swipe but distinct start zone.
- **Start Constraint:** Must START < 20% from a specific edge.
- **Direction:** The closest edge at the start defines the event (e.g., "Left Edge Drag" if starting from x < 20% width).
- **Outcome:** Fires on release or continuously (design choice: usually on release for gestures, or continuously for drawers. Specification implies "reported from" suggesting a discrete gesture, but "Drag" implies continuous. Let's assume continuous reporting or final event. *Clarification from prompt:* "Similar to a swipe..." suggests it might be a discrete gesture, but "Drag" usually implies interaction. The prompt groups it with Swipe in terms of definitions. We will treat it as a high-level gesture event fired on release/completion for this iteration, unless "Hold And Drag" logic applies).
- **Refinement:** The prompt says "Similar to a swipe but must start < 20% away...". This implies it's an "Edge Swipe" gesture.

## 3. Data Interface

### 3.1 Event Structure
The engine must publish events containing:
- **Type:** `TOUCH_TAP`, `TOUCH_HOLD`, `TOUCH_HOLD_DRAG`, `TOUCH_SWIPE`, `TOUCH_EDGE_DRAG`
- **Direction:** `UP`, `DOWN`, `LEFT`, `RIGHT`, `NONE` (for Swipes/Edge Drags)
- **Position (Absolute):** `x_px`, `y_px`
- **Position (Relative):** `x_percent` (0.0-1.0), `y_percent` (0.0-1.0)

## 4. Scenarios

### Scenario: Detecting a Tap
- Given the user presses at (50%, 50%)
- And releases 500ms later at (51%, 51%)
- Then a `TOUCH_TAP` event is generated

### Scenario: Detecting a Hold
- Given the user presses at (50%, 50%)
- And holds for 1.1 seconds within 5% movement
- Then a `TOUCH_HOLD` event is generated

### Scenario: Detecting a Swipe
- Given the user presses at (50%, 50%) (Center, > 20% from edge)
- And moves quickly to (95%, 50%) (> 40% distance)
- And releases
- Then a `TOUCH_SWIPE` event with direction `RIGHT` is generated

### Scenario: Detecting an Edge Drag
- Given the user presses at (5%, 50%) (Left Edge, < 20%)
- And moves to (50%, 50%)
- And releases
- Then a `TOUCH_EDGE_DRAG` event with direction `LEFT` (originating edge) is generated

### Scenario: Relative Coordinates
- Given the screen is 240 pixels wide
- And a touch event occurs at x=120
- Then the reported `x_percent` should be 0.50
