# Feature: Configuration Injection System

> Label: "Config Injection System"
> Category: "System Architecture"
> Prerequisite: features/arch_infrastructure.md

## Introduction

This feature defines the mechanism for injecting sensitive configuration (Wi-Fi credentials) into the firmware at build time without committing them to version control.

## Specification

### 1. The Local Source (`config.json`)
A `config.json` file must exist in the project root with the following structure:
```json
{
  "wifi": [
    {"ssid": "SSID_1", "password": "PASS_1"},
    {"ssid": "SSID_2", "password": "PASS_2"}
  ]
}
```

### 2. The Build-Time Bridge
A PlatformIO extra script (Python) will:
1. Read `config.json` if it exists.
2. If missing, use empty strings or "DEMO_MODE" values.
3. Inject the values as compiler flags:
    * It generates a C-style array of structs or a delimited string to pass all APs.
    * Example: `-DLPAD_WIFI_CONFIG='{"SSID1", "PASS1"}, {"SSID2", "PASS2"}'` (or similar depending on implementation efficiency).
    * `-DLPAD_WIFI_COUNT=2`

### 3. Usage in Code
The credentials will be accessed via the injected macros and managed by a WiFi Manager:
```c
// Example usage in initialization
for (int i=0; i < LPAD_WIFI_COUNT; i++) {
    // try to connect...
}
```

### 4. Security
* `config.json` MUST be added to `.gitignore`.
* `config.example.json` MUST be provided as a template.
