> Prerequisite: features/hal_spec_display.md

# Feature: Configure Device-Specific Display Rotation

> Label: "Target Rotation"
> Category: "Hardware Layer"

This feature provides a method for configuring screen rotation for a specific hardware target at compile time using build flags.

## Scenario: Rotate T-Display S3 Plus to Landscape

**Given** the `tdisplay_s3_plus` build environment defined in `platformio.ini`,

**When** the following changes are applied:
1.  A build flag is added to the `tdisplay_s3_plus` environment to define the rotation angle: `-DAPP_DISPLAY_ROTATION=90`.
2.  The application startup logic in `src/main.cpp` is modified to act on this flag.

**Then** the application must meet these conditions upon startup:
1.  It should check if the `APP_DISPLAY_ROTATION` macro is defined.
2.  If it is defined, the application must call the `display->setRotation()` method with the value from the macro immediately after the display is initialized.
3.  As a result, the display on the T-Display S3 Plus device will be initialized in a landscape orientation, and all subsequent drawing operations will respect the new dimensions.

### Example `main.cpp` Logic
This demonstrates how the application entrypoint should be modified to apply the rotation.

```cpp
#include "hal/display.h" // Or a factory header that provides the display instance
#include <memory>

// Assume a global or accessible 'display' instance, created based on the build target
extern std::unique_ptr<IDisplay> display;

void setup() {
  // ... other setup code, including display initialization

#ifdef APP_DISPLAY_ROTATION
  display->setRotation(APP_DISPLAY_ROTATION);
#endif

  // ... rest of setup can now use the rotated display
}

void loop() {
  // ...
}
```
