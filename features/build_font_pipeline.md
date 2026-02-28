# Feature: Font Pipeline

> Label: "Build: Font Pipeline"
> Category: "Build Pipeline"
> Prerequisite: features/policy_build_pipeline.md

[TODO]

## 1. Overview

The font pipeline (`scripts/generate_theme_fonts.sh`) compiles TrueType/OpenType font files into Adafruit GFX-compatible C headers using the `fontconvert` tool. It produces 5 font headers at prescribed point sizes covering three typeface families (JetBrains Mono, Inter, Outfit). Generated headers are committed to the repository and only regenerated when source font files change.

---

## 2. Requirements

### 2.1 Font Compilation

- 5 font files generated from 3 source typefaces at specified sizes:
  - SystemUI 9pt, SystemUI 18pt (JetBrains Mono).
  - General 12pt, General 24pt (Inter).
  - Logo 48pt (Outfit).
- Output directory: `src/themes/default/fonts/`.
- Naming convention: `Font_<Category>_<Size>pt7b.h`.

### 2.2 Tool Resolution

- Accepts optional path to `fontconvert` binary as first argument.
- Fallback search: `./lib/Adafruit-GFX-Library/fontconvert/fontconvert`, then `./fontconvert`, then PATH.
- Prints build instructions if no executable `fontconvert` is found.

### 2.3 Error Handling

- Individual `fontconvert` failures produce empty output files (no `set -e` on individual calls).
- Missing source font directory is not explicitly checked.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: All 5 font headers generated

    Given fontconvert is available and source fonts exist
    When generate_theme_fonts.sh is run
    Then 5 .h files are created in src/themes/default/fonts/

#### Scenario: Missing fontconvert prints instructions

    Given fontconvert is not found in any search path
    When generate_theme_fonts.sh is run
    Then it prints build instructions and exits with code 1

#### Scenario: Custom fontconvert path accepted

    Given fontconvert exists at /custom/path/fontconvert
    When generate_theme_fonts.sh /custom/path/fontconvert is run
    Then it uses the provided path

### Manual Scenarios (Human Verification Required)

None.
