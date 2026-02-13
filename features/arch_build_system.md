# Architectural Policy: Build System

> Label: "Build System Policy"
> Category: "ARCHITECTURES"
> Prerequisite: features/arch_infrastructure.md

## 1. Toolchain: PlatformIO
This project uses PlatformIO as its primary build and environment management system. All hardware targeting and dependency management MUST be defined in `platformio.ini`.

## 2. Environment Naming Conventions
*   **Base Environments:** Primary hardware targets use simple names (e.g., `esp32s3`, `tdisplay_s3_plus`).
*   **Milestone Demos:** Historical checkpoints use the pattern `demo_<version>_<board>` (e.g., `demo_v05_esp32s3`).

## 3. Source Filter Mandates
To maintain clean separation between hardware targets and historical demos, `build_src_filter` MUST be used rigorously.

### 3.1 Base Hardware Environments
These environments ALWAYS target the latest development state using `main.cpp`.
*   **Include:** `+<main.cpp>`, all active HAL implementations (`+<../hal/*.cpp>`), and the latest demo coordinator.
*   **Exclude:** All HAL stubs (`-<../hal/*_stub.cpp>`) and legacy demo files not used by the current release.

### 3.2 Historical Demo Environments
*   **Exclusion:** Typically exclude `main.cpp` if they use an older monolithic entry point, or use specific build flags to redirect the `main.cpp` dispatcher.

## 4. Hardware Upload Protocol
*   **User Control:** Flashing, uploading, and erasing hardware is a **Human-only** operation.
*   **Builder Role:** The Builder is responsible for building the firmware (`pio run -e <env>`) and providing the exact command for the User to execute for uploading.

## 5. Build-Time Configuration
*   **Dispatcher Pattern:** The application uses build flags (e.g., `-DDEMO_V070`) to select the active demo/app at compile time within `main.cpp`.
*   **Macro Injection:** Secret management and global constants are injected via `scripts/inject_config.py` using compiler defines.
