# Feature: Configuration Injection System

> Label: "Config Injection System"
> Category: "Application Layer"
> Prerequisite: None

## Introduction

This feature defines the mechanism for injecting sensitive configuration (Wi-Fi credentials) into the firmware at build time without committing them to version control.

## Specification

### 1. The Local Source (`config.json`)
A `config.json` file must exist in the project root with the following structure:
```json
{
  "wifi_ssid": "YOUR_SSID",
  "wifi_password": "YOUR_PASSWORD"
}
```

### 2. The Build-Time Bridge
A PlatformIO extra script (Python) will:
1. Read `config.json` if it exists.
2. If missing, use empty strings or "DEMO_MODE" values.
3. Inject the values as compiler flags:
    * `-DLPAD_WIFI_SSID="ValueFromJSON"`
    * `-DLPAD_WIFI_PASSWORD="ValueFromJSON"`

### 3. Usage in Code
The credentials will be accessed via the injected macros:
```c
hal_network_init(LPAD_WIFI_SSID, LPAD_WIFI_PASSWORD);
```

### 4. Security
* `config.json` MUST be added to `.gitignore`.
* `config.example.json` MUST be provided as a template.
