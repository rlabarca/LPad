# HAL Specification: Touch Input

> Label: "HAL: Touch Spec"
> Category: "Hardware Layer"
> Prerequisite: features/hal_core_contract.md

## 1. Introduction
This feature defines the hardware abstraction layer (HAL) for touch input devices. It provides a standard interface for initializing the touch controller and retrieving raw touch coordinates, isolating the application logic from specific touch controller hardware (e.g., CST816).

## 2. Interface Contract

### 2.1 Initialization
The HAL must provide a function to initialize the touch subsystem.

```cpp
/**
 * Initialize the touch hardware.
 * @return true if initialization was successful, false otherwise.
 */
bool hal_touch_init(void);
```

### 2.2 Data Structures
The HAL must define a structure for reporting raw touch status.

```cpp
typedef struct {
    int16_t x;          // X coordinate in screen pixels
    int16_t y;          // Y coordinate in screen pixels
    bool is_pressed;    // true if finger is currently down
} hal_touch_point_t;
```

### 2.3 Polling
The HAL must provide a function to query the current touch state.

```cpp
/**
 * Read the current state of the touch panel.
 * @param point Pointer to a hal_touch_point_t structure to fill.
 * @return true if the read was successful (even if not pressed), false on hardware error.
 */
bool hal_touch_read(hal_touch_point_t* point);
```

## 3. Implementation Constraints
- **Non-Blocking:** `hal_touch_read` must return immediately. It should not use `delay()` or blocking I2C calls that would stall the main loop.
- **Coordinate Mapping:** The `x` and `y` coordinates returned must match the display's pixel coordinate system (0,0 at top-left). If the display is rotated, the touch coordinates must be transformed to match.
- **Dependency:** Implementations must not depend on high-level application logic.

## 4. Scenarios

### Scenario: Initialization Success
- Given the hardware is connected and powered
- When `hal_touch_init()` is called
- Then it should return `true`

### Scenario: Reading Touch Coordinates
- Given the system is initialized
- And a finger is touching the screen at specific coordinates
- When `hal_touch_read()` is called
- Then `point->is_pressed` should be `true`
- And `point->x` and `point->y` should match the touch location

### Scenario: No Touch Detected
- Given the system is initialized
- And no finger is touching the screen
- When `hal_touch_read()` is called
- Then `point->is_pressed` should be `false`
