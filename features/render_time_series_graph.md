# Feature: Time Series Graph

> Label: "Render: Time Series Graph"
> Category: "Rendering"
> Prerequisite: features/arch_display_pipeline.md
> Prerequisite: features/arch_concurrency.md

[TODO]

## 1. Overview

The TimeSeriesGraph renders financial time-series data using a three-layer PSRAM canvas architecture: a static background layer (gradient, axes, ticks, watermark), a data layer (line chart, redrawn on data update), and a composite layer (background + data merged, then live indicator animated on top). All layers use off-screen PSRAM canvases composited via atomic DMA blit to prevent tearing. The graph supports configurable margins, tick positions (inside/outside axes), axis titles, and watermark text.

---

## 2. Requirements

### 2.1 Three-Layer Architecture

- Background canvas: static content (gradient/solid fill, axes, Y/X ticks, titles, watermark). Redrawn only on theme change or data range change.
- Data canvas: data line drawn on chroma key background (0x0001). Redrawn on each data update.
- Composite: background + data merged in a PSRAM buffer, then DMA blit to display. Live indicator drawn on top each frame.

### 2.2 PSRAM Requirement

- `begin()` MUST require PSRAM. Returns false if PSRAM is unavailable.
- All three canvases (background, data, composite buffer) are PSRAM-allocated.

### 2.3 Coordinate Mapping

- `mapYToScreen(y, ymin, ymax)` maps a Y value to relative screen percentage (inverted: high values = top).
- `mapXToScreen(index, count)` maps a data point index to relative screen percentage within margins.

### 2.4 Y Tick Algorithm

- Round `y_min` up to the first clean multiple of the tick increment.
- Generate ticks as integer multiples to avoid float drift.
- Suppress ticks within 8% of the X-axis (origin suppression).
- Iteratively increase tick skip until all labels (3 significant digits) are unique.

### 2.5 Live Indicator

- Pulsing radial-gradient dot at the last data point.
- Radius oscillates using smoothstep easing: `t = (sin(phase)+1)/2`, `radius = min + (max-min) * t^2(3-2t)`.
- Atomic blit: copies background from composite buffer into a region buffer, draws new indicator, blits the combined result to prevent tearing.
- Tracks previous indicator position for erase box calculation.

### 2.6 Graph Theme

- All visual parameters (colors, gradients, line thickness, indicator pulse speed, watermark color) are configured via the `GraphTheme` struct.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Begin fails without PSRAM

    Given PSRAM is not available
    When begin() is called
    Then it returns false

#### Scenario: Data line redrawn on new data

    Given the graph has been initialized
    When setData is called with new GraphData
    And drawData is called
    Then the data canvas is cleared and the line is redrawn

#### Scenario: Y tick labels are unique

    Given a Y range that produces multiple ticks at the default increment
    When drawBackground calculates ticks
    Then all rendered tick labels have unique 3-significant-digit values

#### Scenario: Origin suppression hides near-axis ticks

    Given a tick falls within 8% of the X-axis position
    When drawBackground renders Y ticks
    Then that tick is not drawn

#### Scenario: Live indicator pulses with smoothstep easing

    Given the graph is rendering with data
    When update(dt) is called repeatedly
    Then the live indicator radius oscillates smoothly between min and max

### Manual Scenarios (Human Verification Required)

#### Scenario: Graph renders correctly on device

    Given the device is displaying live stock data
    Then the chart shows gradient background, axis labels, data line, and pulsing indicator
    And no tearing or flicker is visible during animation
