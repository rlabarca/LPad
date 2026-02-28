# Feature: Build Version Injection

> Label: "Build: Version Injection"
> Category: "Build Pipeline"
> Prerequisite: features/policy_build_pipeline.md

[TODO]

## 1. Overview

The firmware version is defined once in `platformio.ini` and compiled into a C preprocessor macro via a PlatformIO pre-build script. All application code references this macro instead of hardcoded version strings, ensuring single-source-of-truth for version bumps.

---

## 2. Requirements

### 2.1 Version Source

- The firmware version is defined in `platformio.ini` under the `[platformio]` section as `custom_lpad_version = 0.76`.
- The value is a semantic version string (no "v" prefix).
- This is the single source of truth for the firmware version.

### 2.2 Generated Header

- A pre-build script generates `src/generated/lpad_version.h` containing `#define LPAD_VERSION "0.76"`.
- The file uses an include guard: `LPAD_VERSION_H`.
- Uses idempotent write: the file is only rewritten if content differs from the current file on disk (prevents unnecessary rebuilds). This follows the same pattern as `inject_config.py`.

### 2.3 Script

- `scripts/inject_version.py` is a PlatformIO pre-build hook.
- Reads the version via `env.GetProjectConfig().get("platformio", "custom_lpad_version")`.
- Falls back to `"0.0.0-unknown"` if the `custom_lpad_version` field is missing.
- Uses PlatformIO `Import("env")` SCons API; cannot be run standalone.

### 2.4 Integration

- Registered as `extra_scripts` in all three PlatformIO environments: `native_test`, `esp32s3`, and `tdisplay_s3_plus`.
- For hardware environments (`esp32s3`, `tdisplay_s3_plus`), added alongside the existing `inject_config.py` entry.
- For `native_test`, added as the sole pre-build script (inject_config.py is not used in native_test).

### 2.5 Code Migration

- All hardcoded version string literals in `src/main.cpp` are replaced with the `LPAD_VERSION` macro using C string literal concatenation (e.g., `"LPad v" LPAD_VERSION`).
- The `setVersion()` call uses `"Version " LPAD_VERSION`.
- The `@brief` file comment drops the version number entirely (comments are not functional code).
- After migration, zero hardcoded version strings remain in application source code. Verification: `grep -r "v0\.\|Version 0\." src/` returns zero matches.

### 2.6 Gitignore

- `src/generated/lpad_version.h` is added to `.gitignore` (generated artifact, reproducible from `platformio.ini`).

---

## 3. Scenarios

### Automated Scenarios

#### Scenario: Missing custom_lpad_version falls back to unknown

    Given platformio.ini does not contain the custom_lpad_version field
    When the inject_version.py pre-build script runs
    Then lpad_version.h contains #define LPAD_VERSION "0.0.0-unknown"

#### Scenario: Generated header contains correct LPAD_VERSION define

    Given platformio.ini contains custom_lpad_version = 0.76
    When the inject_version.py pre-build script runs
    Then lpad_version.h contains #define LPAD_VERSION "0.76"
    And the file includes an LPAD_VERSION_H include guard

#### Scenario: Idempotent write does not rewrite unchanged header

    Given lpad_version.h already exists with the correct content
    When the inject_version.py pre-build script runs
    Then the file is not rewritten (modification time unchanged)

#### Scenario: LPAD_VERSION macro expands in application code

    Given the firmware is compiled with custom_lpad_version = 0.76
    When main.cpp references LPAD_VERSION
    Then the macro expands to "0.76" in the compiled output

#### Scenario: All main.cpp Serial output includes the version from the macro

    Given the firmware boots
    When the setup() function prints the startup banner
    Then the banner includes the version string from LPAD_VERSION
    And no hardcoded version literals exist in main.cpp

### Manual Scenarios (Human Verification Required)

None.
