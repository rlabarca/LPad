# Feature: Release 0.55 Demo Application

> Label: "Demo for Release v0.55"
> Category: "Release Demos"

> **Prerequisite:** `features/demo_release_0.5.md`
> **Prerequisite:** `features/ui_connectivity_status_screen.md`
> **Prerequisite:** `features/app_config_system.md`

This feature defines the demo application used to validate Release v0.55. It extends the v0.5 visual demo by prepending a connectivity check and ensuring the system remains functional after the check.

## Architecture: `V055DemoApp` Class

The `V055DemoApp` class coordinates the transition between connectivity validation and the visual demo.

### Composition:
-   **`V05DemoApp`**: Encapsulates all rendering logic from Release 0.5 (Logo Animation + 6 Graph Modes).
-   **`ConnectivityStatusScreen`**: Manages the Wi-Fi/Ping visual feedback.

### Public Interface:
1.  **`begin(RelativeDisplay* display)`**: 
    -   Initializes the `ConnectivityStatusScreen`.
    -   Initializes the `V05DemoApp` (but does not start its animation yet).
    -   Configures the `V05DemoApp` to display the title "**DEMO v0.55**".
    -   Starts the Wi-Fi connection process via the HAL.
2.  **`update(float deltaTime)`**: 
    -   **Phase 1 (Connectivity):** Updates `ConnectivityStatusScreen` with current network status and ping results.
    -   **Phase 2 (Handover):** Once "PING OK" has been displayed for 2 seconds, it transitions to Phase 3.
    -   **Phase 3 (Visual Demo):** Calls `V05DemoApp::update(deltaTime)`.
3.  **`render()`**:
    -   Renders the `ConnectivityStatusScreen` during Phase 1 & 2.
    -   Renders the `V05DemoApp` during Phase 3.

## Scenario: Connectivity Transition

**Given** the device has booted and `V055DemoApp` has started.
**When** the Wi-Fi is connecting.
**Then** the `ConnectivityStatusScreen` should display "CONNECTING...".

**When** the ping to "8.8.8.8" is successful.
**Then** the screen should display "PING OK".
**And** after 2 seconds, the screen should transition to the `V05DemoApp` (starting with the Logo Animation, then cycling through all 6 graph modes).

## Scenario: Regression Verification

**Given** the demo has transitioned to the `V05DemoApp`.
**When** the v0.5 demo cycle completes (`isFinished()` returns true after all 6 graph modes).
**Then** the `V055DemoApp` SHOULD restart the connectivity check to verify ongoing stability (returning to Phase 1).

## Hardware (HIL) Test

To visually confirm the correct operation of this feature, the `src/main.cpp` file must be temporarily modified to implement the demo logic using the `V055DemoApp` class.

**Instructions for the Builder:**

1.  **Implement `V055DemoApp`:** Create `demos/v055_demo_app.h` and `.cpp`. It should compose/inherit from `V05DemoApp`.
2.  **Modify `src/main.cpp`:** Instantiate and use `V055DemoApp` directly.
3.  **Verify:** 
    -   Verify the transition from Connectivity Screen -> Logo -> Graph.
    -   Verify that credentials from `config.json` are used correctly.
    -   Verify that the full cycle repeats correctly.
