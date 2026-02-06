# Feature: Connectivity Status Screen

> Label: "Connectivity Status Screen"
> Category: "UI Framework"
> Prerequisite: features/hal_spec_network.md, features/ui_base.md

## Introduction

This feature adds a new screen to the system's screen rotation that displays the status of the network connection and the results of the "Smoke Test" ping.

## UI Scenarios

### Scenario: Connecting to Network
*   **Context:** `hal_network_get_status()` returns `HAL_NETWORK_STATUS_CONNECTING`.
*   **UI Display:** Shows "CONNECTING..." centered on the screen.
*   **Typography:** `FONT_NORMAL`.
*   **Colors:** `THEME_COLOR_TEXT_PRIMARY` on `THEME_COLOR_BG_PRIMARY`.

### Scenario: Ping Success (Smoke Test Pass)
*   **Context:** `hal_network_get_status()` is `CONNECTED` AND `hal_network_ping()` was successful.
*   **UI Display:** Shows **"PING OK"** centered on the screen.
*   **Typography:** `FONT_TITLE` (48pt).
*   **Colors:** `THEME_COLOR_TEXT_PRIMARY` on `THEME_COLOR_BG_PRIMARY`.

### Scenario: Failure (WIFI or Ping Error)
*   **Context:** `hal_network_get_status()` is `ERROR` OR ping failed.
*   **UI Display:** Shows "CONNECTIVITY ERROR" or "PING FAILED".
*   **Typography:** `FONT_NORMAL`.
*   **Colors:** `THEME_COLOR_TEXT_PRIMARY` on `THEME_COLOR_BG_PRIMARY`.

## Integration
*   The screen must be integrated into the existing `demo_screen.cpp` rotation.
*   It should be the first screen shown while connection is in progress.
