# Release v0.55 - Connectivity Smoke Test

> Label: "Release v0.55 - Connectivity Smoke Test"
> Category: "Releases"
> Prerequisite: features/RELEASE_v0.5_display_drawing_ui_base.md
> Prerequisite: features/hal_spec_network.md
> Prerequisite: features/app_config_system.md
> Prerequisite: features/ui_connectivity_status_screen.md

## 1. Release Capability Description
This release validates the system's ability to securely manage Wi-Fi credentials, establish a network connection on ESP32 hardware, and verify internet reachability via a ping. It bridges the gap between the static UI (v0.5) and dynamic data (v0.6).

## 2. Integration Test Criteria

### Success Metrics
1.  **Secure Build:** The firmware compiles correctly using credentials from `config.json` without those credentials appearing in the source code.
2.  **Network Init:** The device successfully initiates a Wi-Fi connection to the configured SSID.
3.  **UI Feedback:** The "Connectivity Status Screen" correctly transitions from "CONNECTING..." to **"PING OK"** upon successful internet verification.
4.  **Regression:** The existing v0.5 screens (Logo, Graph, Live Indicator) remain functional and appear in the screen rotation after the connectivity check.
5.  **Robustness:** If incorrect credentials are provided in `config.json`, the UI displays a clear error message.
