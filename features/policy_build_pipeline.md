# Policy: Build Pipeline

> Label: "Policy: Build Pipeline"
> Category: "Policy"

## Purpose

Defines the build and code generation policies for the LPad firmware project. The build system (PlatformIO) targets three environments: two embedded (Waveshare, LilyGo) and one host-native for unit testing. Build-time configuration injection ensures sensitive credentials never appear in source code. Two offline code generation pipelines (SVG assets, font compilation) produce committed artifacts that are regenerated only when source assets change.

## Build Pipeline Invariants

### PlatformIO Environment Rules

- Three build environments MUST be maintained: `native_test` (host unit tests), `esp32s3` (Waveshare board), `tdisplay_s3_plus` (LilyGo board).
- HAL implementation selection is exclusively via `build_src_filter` in `platformio.ini`. Each environment includes only its target's implementation files and excludes all others.
- The `native_test` environment uses only `_stub.cpp` HAL implementations and MUST compile without any embedded SDK.

### Configuration Injection

- WiFi credentials are stored in a gitignored `config.json` at the project root.
- The PlatformIO pre-build script (`scripts/inject_config.py`) reads `config.json` and generates `src/wifi_config_generated.h` at build time.
- `config.json` MUST NOT be committed to git. `config.example.json` provides the template.
- The injection script supports both multi-WiFi format (`{"wifi": [...]}`) and legacy single-WiFi format.

### Code Generation Pipelines

- **SVG Pipeline** (`scripts/process_svgs.py`): Converts `assets/*.svg` to triangulated C++ mesh data in `src/generated/vector_assets.h/.cpp`. Generated files ARE committed. Re-run only when SVG source assets change.
- **Font Pipeline** (`scripts/generate_theme_fonts.sh`): Converts TTF/OTF fonts to GFX-compatible C headers in `src/themes/default/fonts/`. Generated files ARE committed. Re-run only when font source files change.
- **Config Pipeline** (`scripts/inject_config.py`): Generates `src/wifi_config_generated.h` at every build. Generated file is NOT committed (gitignored).

### Test Framework

- Unit tests use the Unity test framework on the `native_test` PlatformIO environment.
- Test runner script (`scripts/test_local.sh`) outputs JSON summary to `.pio/testing/last_summary.json`.
- Tests that depend on PSRAM or Arduino GFX are excluded from `native_test` via `test_ignore` in `platformio.ini`.

## Scenarios

No automated or manual scenarios. This is a policy anchor node -- its "scenarios" are
process invariants enforced by instruction files and tooling.
