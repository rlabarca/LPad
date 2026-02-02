> Prerequisite: `features/display_relative_drawing.md`

# Feature: Display Rotation Contract

This feature adds screen rotation capabilities to the core display HAL contract. It ensures that all display drivers can be instructed to rotate their output and that higher-level modules can query the resulting dimensions.

## Scenario: Add Rotation Method to IDisplay Interface

**Given** the `IDisplay` interface defined in `hal/display.h`,

**When** the following changes are applied:
1.  A new pure virtual method `virtual void setRotation(int degrees) = 0;` is added to the `IDisplay` interface.
2.  All existing concrete HAL implementations are updated to implement this new method.

**Then** the following conditions must be met:
1.  The `setRotation` method in each concrete implementation (`hal/display_tdisplay_s3_plus.cpp`, `hal/display_esp32_s3_amoled.cpp`) must call the underlying graphics library's rotation command.
2.  After `setRotation` is called on a display instance, subsequent calls to its `getWidth()` and `getHeight()` methods must return the dimensions corresponding to the new orientation.
3.  The `hal/display_stub.cpp` implementation must be updated:
    *   It should store the original width and height provided during construction.
    *   Its `setRotation` method should store the given rotation.
    *   Its `getWidth()` and `getHeight()` methods must calculate and return the correct dimension (swapping if rotation is 90 or 270) based on the stored original dimensions and the current rotation.
