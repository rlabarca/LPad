# Feature: HAL Core Contract

> Label: "HAL Core Contract"
> Category: "Hardware Layer"
> Prerequisite: features/arch_hal_policy.md

## Introduction

The HAL Core Contract defines the fundamental architectural rules and coding standards that all specific Hardware Abstraction Layer (HAL) modules (Display, Network, Timer, etc.) must follow. It ensures a consistent, portable, and testable interface between the Application Layer and the physical hardware.

## Fundamental Principles

### 1. C Compatibility
*   **Requirement:** All HAL public headers MUST be valid C headers (using `extern "C"` if implemented in C++).
*   **Reason:** Ensures maximum compatibility with different toolchains and simplifies linking between different language modules.
*   **Constraint:** Public HAL headers MUST NOT use C++ specific features like classes, namespaces (outside of prefixing), or the C++ Standard Template Library (STL).

### 2. Error Handling
*   **Pattern:** Initialization functions and operations that can fail MUST return a boolean (`true` for success, `false` for failure) or a specific status enum.
*   **Logging:** HAL implementations may log errors to serial/console, but the contract itself should remain silent unless an error occurs.

### 3. The Stub Mandate
*   **Requirement:** For every `hal/domain.h` header and `hal/domain_target.cpp` implementation, there MUST be a corresponding `hal/domain_stub.cpp`.
*   **Functionality:** Stubs must provide no-op or simulated behavior that allows the application to compile and execute logic tests on a host PC (native environment).

### 4. Non-Blocking by Default
*   **Requirement:** Long-running operations (Network connection, large display flushes) should be asynchronous or non-blocking where possible.
*   **Synchronization:** If a blocking call is required, it must be clearly documented as such (e.g., `hal_display_flush`).

### 5. Dependency Isolation
*   **Requirement:** HAL headers MUST NOT include vendor-specific headers (e.g., `esp_wifi.h`, `TFT_eSPI.h`).
*   **Abstraction:** Use opaque handles (`void*`) or standard types (`uint32_t`, etc.) to pass hardware-specific data if absolutely necessary.

## API Naming Convention

All HAL functions must follow the prefix pattern: `hal_<domain>_<action>`.
*   Example: `hal_display_init()`, `hal_network_connect()`.

## Implementation Notes

### [2026-02-06] Domain-Segmented HAL Architecture
As the project expanded into Networking (v0.6), the monolithic `hal_contracts.md` was split into domain-specific specs (`hal_spec_display.md`, `hal_spec_timer.md`, `hal_spec_network.md`). Each domain has its own header to prevent cross-domain includes. Refactor HAL *before* it gets messy — segmenting by domain early prevents "Header Spaghetti."

### [2026-02-06] Interactive Graph Viewer
**Problem:** Terminal-rendered Mermaid graphs were not zoomable.
**Solution:** `scripts/serve_graph.py` (Python server) + `scripts/graph_viewer.html` (Mermaid.js + svg-pan-zoom). Live-reloads on changes to `feature_graph.mmd`, preserving zoom level. Browser > Terminal for complex visualizations.

### [2026-02-06] Gherkin Prerequisite Formatting
The dependency parser is sensitive to formatting. Multiple prerequisites MUST be on separate lines, each with its own `> Prerequisite:` prefix.

### [2026-02-05] PlatformIO Include Path Conventions
**Problem:** Demo files in `demos/` with relative includes (`#include "../src/file.h"`) failed to compile.
**Solution:** Use direct includes (`#include "file.h"`). PlatformIO's `-I` flags handle `src/` and `hal/` directories. Trust PlatformIO's conventions.
