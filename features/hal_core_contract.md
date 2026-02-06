# Feature: HAL Core Contract

> Label: "HAL Core Contract"
> Category: "Hardware Layer"
> Prerequisite: None

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
