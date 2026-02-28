# Architecture: HAL Contract

> Label: "Architecture: HAL Contract"
> Category: "Architecture"

## Purpose

Defines the hardware abstraction layer pattern used throughout the LPad firmware. All hardware interaction is isolated behind pure C-function contracts in `hal/*.h`, with multiple concrete implementations selected at compile time via PlatformIO `build_src_filter`. This pattern enables host-native unit testing via stub implementations and supports dual-board targeting without conditional compilation in application code.

## HAL Invariants

### Interface Contract Rules

- Every HAL domain (display, touch, network, power, timer) MUST have a single C header (`hal/<domain>.h`) declaring its contract with `extern "C"` linkage.
- Application code above the HAL boundary MUST NOT include any hardware-specific header (e.g., `Wire.h`, `WiFi.h`, `SPI.h`). All hardware access flows through HAL functions.
- HAL headers MUST NOT include Arduino, ESP-IDF, or any vendor-specific headers. They declare only C types and function prototypes.

### Implementation Selection Rules

- Each HAL contract MUST have at least one real implementation and one stub implementation for native testing.
- Implementation selection is exclusively compile-time via PlatformIO `build_src_filter` in `platformio.ini`. There is no runtime dispatch or polymorphism at the HAL level.
- Stub implementations MUST compile and link on the host-native platform without any embedded SDK dependencies.

### Naming Conventions

- Contract headers: `hal/<domain>.h`
- Real implementations: `hal/<domain>_<target>.cpp` (e.g., `display_esp32_s3_amoled.cpp`)
- Test stubs: `hal/<domain>_stub.cpp`

### I2C Bus Sharing

- Multiple HAL domains (touch, power) share a single I2C bus. Implementations MUST handle bus contention (retries, NACK handling) internally.
- HAL implementations MUST NOT call `Wire.begin()` unconditionally. Shared-bus implementations use callback-based initialization or guard against double-initialization.

## Scenarios

No automated or manual scenarios. This is a policy anchor node -- its "scenarios" are
process invariants enforced by instruction files and tooling.
