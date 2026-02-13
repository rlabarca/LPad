# Feature: WiFi Selection List Widget

> Label: "WiFi Selection List Widget"
> Category: "UI Framework"
> Prerequisite: features/ui_standard_widgets.md
> Prerequisite: features/hal_spec_network.md

## 1. WiFiListWidget
A specialized `ScrollableListWidget` for managing WiFi connections.

### Capabilities
- **Population:** Automatically populated with the list of SSIDs from the compiled configuration.
- **Visual State Tracking:**
    - **Active:** The currently connected network is displayed in a bright highlight color from the theme.
    - **Connecting:** When an entry is tapped, its background color changes to a theme-specified "connecting" color.
    - **Failed:** If connection fails, the background returns to normal and the text turns red.
    - **Connected:** On success, the text turns the bright highlight color and the "connecting" background is removed.
- **Integration:** Calls `hal_network_init(ssid, password)` when a new item is selected.

## Scenario: Selecting a New Network
    Given the WiFiListWidget is visible in the System Menu
    And "HomeWiFi" is currently connected (Bright Highlight)
    When the User taps on "OfficeWiFi"
    Then "OfficeWiFi" background changes to THEME_BG_CONNECTING
    And "HomeWiFi" text color returns to normal
    And `hal_network_init("OfficeWiFi", ...)` is called
    When the connection is successful
    Then "OfficeWiFi" text color becomes THEME_TEXT_HIGHLIGHT
    And "OfficeWiFi" background becomes transparent.

## Implementation Notes
- **State Management:** The widget needs to poll `hal_network_get_status()` or be notified of status changes to update its visual state.
- **Stored Credentials:** The widget must have access to the compiled list of SSID/Password pairs.
- **SSIDChangeCallback:** WiFiListWidget fires a callback when a connection succeeds, allowing SystemMenu to update the SSID overlay display without polling.
- **Disconnect-before-reconnect:** hal_network_init() now calls disconnect first when switching networks, with a 10s connection timeout tracked via millis().
