# Feature: Build Config Injection

> Label: "Build: Config Injection"
> Category: "Build Pipeline"
> Prerequisite: features/policy_build_pipeline.md

[TODO]

## 1. Overview

The config injection system is a PlatformIO pre-build hook (`scripts/inject_config.py`) that reads WiFi credentials from a gitignored `config.json` and generates both C preprocessor build flags and a C++ header file (`src/wifi_config_generated.h`) containing a WiFiEntry array. It supports multi-WiFi (array of networks) and legacy single-WiFi formats, with a DEMO_MODE fallback when no config is found.

---

## 2. Requirements

### 2.1 Config Formats

- Multi-WiFi (canonical): `{"wifi": [{"ssid": "...", "password": "..."}]}`.
- Legacy single-WiFi: `{"wifi_ssid": "...", "wifi_password": "..."}`.
- `wifi` key checked first; entries with empty `ssid` are skipped.
- Missing or malformed `config.json` falls through to DEMO_MODE: single entry `("DEMO_MODE", "")`.

### 2.2 Build Flags

- Three defines injected: `LPAD_WIFI_SSID`, `LPAD_WIFI_PASSWORD` (first entry only), `LPAD_WIFI_COUNT` (total entries).

### 2.3 Generated Header

- Output: `src/wifi_config_generated.h` with `g_wifi_config[]` (WiFiListWidget::WiFiEntry array) and `g_wifi_count`.
- Special characters in SSIDs/passwords are escaped (`\` -> `\\`, `"` -> `\"`).
- File is only written if content differs from current file (prevents unnecessary rebuilds).

### 2.4 Integration

- Registered as `extra_scripts = pre:scripts/inject_config.py` in `platformio.ini` for embedded environments only (not `native_test`).
- Uses PlatformIO `Import("env")` SCons API; cannot be run standalone.
- A sibling pre-build hook (`scripts/inject_version.py`) is registered alongside this script in hardware environments. See `features/build_version_injection.md` for details.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Multi-WiFi config generates correct header

    Given config.json contains 3 WiFi entries
    When the PlatformIO build runs
    Then wifi_config_generated.h contains 3 WiFiEntry structs
    And g_wifi_count equals 3

#### Scenario: Legacy format is supported

    Given config.json uses wifi_ssid/wifi_password keys
    When the PlatformIO build runs
    Then wifi_config_generated.h contains 1 WiFiEntry

#### Scenario: Missing config falls back to DEMO_MODE

    Given config.json does not exist
    When the PlatformIO build runs
    Then wifi_config_generated.h contains ("DEMO_MODE", "")

#### Scenario: Special characters are escaped

    Given a WiFi password containing backslash and double-quote
    When the header is generated
    Then the characters are properly escaped in the C string

#### Scenario: Idempotent write prevents rebuild

    Given the generated header content has not changed
    When the build runs
    Then the file is not rewritten (modification time unchanged)

### Manual Scenarios (Human Verification Required)

None.
