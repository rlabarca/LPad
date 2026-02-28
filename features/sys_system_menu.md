# Feature: System Menu

> Label: "System: System Menu"
> Category: "System Components"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/design_visual_language.md

[TODO]

## 1. Overview

The SystemMenuComponent is a SystemComponent (Z=20, highest Z) that provides a slide-in overlay panel activated by a top-edge downward drag. It wraps an inner SystemMenu class that manages a 250ms open/close animation, a WiFi network selector (WiFiListWidget), battery status display, connected SSID, and firmware version string. The menu is dismissed by a bottom-edge upward drag or the physical home button.

---

## 2. Requirements

### 2.1 Activation and Dismissal

- Activated by `EDGE_DRAG UP` (top-edge drag downward) via UIRenderManager's activation routing.
- Dismissed by `EDGE_DRAG DOWN` (bottom-edge drag upward) while the menu is OPEN. Calls `close()` and sets closing flag.
- Home button input is synthesized as `EDGE_DRAG DOWN` in the main loop, triggering the same close path.
- After close animation completes (state returns to CLOSED), `systemPause()` is called to return control to the active app.

### 2.2 Animation

- OPENING: progress 0.0 -> 1.0 at rate `1.0 / ANIMATION_DURATION` (0.25s = 4.0 per second).
- CLOSING: progress 1.0 -> 0.0 at the same rate.
- During animation (OPENING/CLOSING), widgets are NOT rendered. Only the sliding background panel is visible.
- When OPEN (progress = 1.0), widgets render clipped to the visible height.

### 2.3 Content Layout

- 5-row x 1-col GridWidgetLayout anchored at top-center with 15% vertical offset, 50% width, 70% height.
- Row 0: TextWidget heading "WiFi Networks".
- Rows 1-4: WiFiListWidget (4-row span).
- Battery status: top-left corner, color-coded (green=charging, red=low <15%, normal otherwise). "NO BATTERY" for no-battery state.
- SSID: top-right corner, refreshed via SSIDProvider callback on each open.
- Version: bottom-center.

### 2.4 Input Handling

- While OPEN: all input is forwarded to the inner widget system. All events return `true` (consumed) to prevent pass-through to the app.
- While not OPEN (animating or closed): all events still return `true` to prevent unintended app interaction.

### 2.5 Rendering

- Inner SystemMenu uses a PSRAM canvas for atomic DMA blit (no tearing during animation).
- Dirty flag prevents redundant redraws. `render()` is no-op when not dirty.
- Background: `m_bgColor` for the menu panel, `m_revealColor` for the area below (matches app background).

### 2.6 Component Properties

- `isOpaque()` returns `true`. `isFullscreen()` returns `true`.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Top-edge drag opens menu

    Given the system menu is closed (hidden, paused)
    When an EDGE_DRAG UP event is routed to the UIRenderManager
    Then the SystemMenuComponent receives onUnpause
    And the inner menu transitions to OPENING state

#### Scenario: Bottom-edge drag closes menu

    Given the system menu is OPEN
    When an EDGE_DRAG DOWN event is received
    Then the inner menu transitions to CLOSING state

#### Scenario: Close animation triggers systemPause

    Given the inner menu is in CLOSING state
    When the animation completes (progress reaches 0.0)
    Then the state transitions to CLOSED
    And systemPause is called to restore the active app

#### Scenario: All input consumed while open

    Given the system menu is OPEN
    When any touch event is received
    Then handleInput returns true (event consumed)
    And the active app does not receive the event

#### Scenario: Widgets not rendered during animation

    Given the system menu is in OPENING state
    When render is called
    Then only the sliding background is drawn
    And no widgets are rendered

#### Scenario: SSID refreshed on each open

    Given an SSIDProvider callback is configured
    When the system menu opens (onUnpause)
    Then the SSIDProvider is called to get the current SSID

### Manual Scenarios (Human Verification Required)

#### Scenario: System menu slide-in animation on device

    Given the device is showing the stock chart
    When the user swipes down from the top edge
    Then the system menu slides in from the top over 250ms
    And WiFi networks, battery status, SSID, and version are displayed
    And swiping up from the bottom closes the menu

#### Scenario: WiFi network selection from menu

    Given the system menu is open
    When the user taps a WiFi network name
    Then the entry blinks during connection
    And the connected SSID updates in the menu header
