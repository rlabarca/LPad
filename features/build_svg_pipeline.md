# Feature: SVG Pipeline

> Label: "Build: SVG Pipeline"
> Category: "Build Pipeline"
> Prerequisite: features/policy_build_pipeline.md

[TODO]

## 1. Overview

The SVG pipeline (`scripts/process_svgs.py`) converts SVG files from the `assets/` directory into C++ source code containing triangulated vector mesh data. Each SVG `<path>` element with a `fill` color is treated as a single triangle (first 3 vertices). Output is a pair of generated files (`src/generated/vector_assets.h/.cpp`) with normalized (0-1) vertex coordinates and RGB565 colors, ready for rendering by VectorRenderer.

---

## 2. Requirements

### 2.1 Input Format

- Processes all `*.svg` files in `assets/` (alphabetical order).
- SVG `<path>` elements MUST have a `fill` attribute (hex `#RRGGBB`). Paths with `fill="none"` or missing fill are skipped.
- Each `<path>` MUST represent exactly one triangle. Only the first 3 vertices from `M`, `L`, and implicit coordinate pairs are used.
- No curve support (`C`, `Q`, `A` commands are not parsed).
- ViewBox dimensions from `viewBox` attribute (or `width`/`height` attributes, default 100x100).

### 2.2 Coordinate Normalization

- All vertex X coordinates divided by SVG width, Y by SVG height, producing values in [0.0, 1.0].

### 2.3 Color Conversion

- Hex `#RRGGBB` converted to RGB565: `(r>>3 << 11) | (g>>2 << 5) | (b>>3)`.

### 2.4 Output Format

- Header (`vector_assets.h`): struct definitions (VectorVertex, VectorTriangle, VectorPath, VectorShape) + namespace `VectorAssets` with extern declarations.
- Source (`vector_assets.cpp`): static triangle arrays, path arrays, and shape constants.
- Shape names derived from filename: `lpad_logo.svg` -> `LpadLogo`.

### 2.5 Error Handling

- No SVG files found: exit code 1.
- No valid shapes after parsing: exit code 1.
- Parse errors per file: warning printed, file skipped.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: SVG with valid paths generates output

    Given assets/test.svg contains 5 triangular paths with fill colors
    When process_svgs.py is run
    Then vector_assets.h declares VectorAssets::Test
    And vector_assets.cpp contains 5 VectorTriangle entries

#### Scenario: Coordinates are normalized to 0-1

    Given an SVG with viewBox "0 0 200 400"
    And a vertex at (100, 200)
    When process_svgs.py generates the output
    Then the vertex is (0.500000, 0.500000)

#### Scenario: Paths without fill are skipped

    Given an SVG path with fill="none"
    When process_svgs.py processes the file
    Then that path is not included in the output

#### Scenario: No SVG files returns exit code 1

    Given the assets/ directory is empty
    When process_svgs.py is run
    Then it exits with code 1

### Manual Scenarios (Human Verification Required)

None.
