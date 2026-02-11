# Feature: Touch Test Overlay

> Label: "UI: Touch Overlay"
> Category: "UI Framework"
> Prerequisite: features/touch_gesture_engine.md

## 1. Introduction
To validate the touch subsystem and gesture engine, a debug overlay will be added. This overlay renders high-level touch event information directly on top of the active screen (e.g., the Stock Tracker Graph) without interfering with the underlying application logic.

## 2. Functional Requirements

### 2.1 Visual Design
- **Font:** Second largest font available in the theme.
- **Background:** The text must be rendered with a background color box (using the theme's background color) to ensure legibility over the graph.
- **Position:** Centered or unobtrusive (Requirement: "displayed over any active screen").
- **Visibility:** Default hidden. Appears only when an event occurs.

### 2.2 Behavior
- **Trigger:** Displays when a high-level event (Tap, Hold, Swipe, etc.) is reported by the Touch Gesture Engine.
- **Timeout:** The overlay must disappear automatically if no new event is registered for 3 seconds.
- **Independence:** The rendering logic must be decoupled from the specific active screen (e.g., implemented as a global hook or a root-level render pass in `main.cpp` or `DisplayManager`).

### 2.3 Content Format
The text must follow the format:
`<EVENT_NAME>: (<X,Y> and/or %)`

Examples:
- `TAP: (120, 50) 50%`
- `SWIPE: RIGHT`
- `HOLD_DRAG: (200, 100)`

## 3. Scenarios

### Scenario: Overlay Appearance
- Given the overlay is currently hidden
- When a `TOUCH_TAP` event occurs
- Then the overlay should appear with text like "TAP: ..."
- And the text should have a background box

### Scenario: Overlay Timeout
- Given the overlay is visible
- When 3 seconds elapse without any new touch events
- Then the overlay should become hidden

### Scenario: Non-Interference
- Given the Stock Tracker graph is updating
- When the overlay is drawn
- Then the graph data underneath should not be corrupted (though it may be visually obscured)
