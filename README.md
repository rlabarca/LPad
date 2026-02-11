# LPad: Agentic Embedded Development

**LPad** is an experimental ESP32 project that serves as a proving ground for a **Spec-Driven Agentic Workflow**.

The primary goal of this repository is not just the firmware itself, but the *process* of building it. We use a rigorous "Two-Agent" model where **Gemini** acts as the Architect and **Claude** acts as the Developer, with **You** (the Human) as the Executive/Process Manager.

## 🤖 The Agentic Workflow

In this project, **Code is Disposable**. The "Source of Truth" is the set of Feature Specifications (`features/*.md`). If we deleted `src/` today, the agents could rebuild it entirely from the specs.

### The Agents

*   **Gemini (The Architect):**
    *   **Role:** Designs the system, writes Feature Specifications (`features/*.md`), enforces architectural constraints, and manages the DevOps process.
    *   **Context:** Uses `GEMINI_ARCHITECT.md` to understand its role and mandates.
    *   **Output:** Creates rigorous Gherkin-style specs and updates `docs/`.
*   **Claude (The Builder):**
    *   **Role:** Writes the C++/PlatformIO code to satisfy the specs.
    *   **Context:** Uses `CLAUDE.md` for coding standards, git protocols, and command usage.
    *   **Output:** Writes `src/`, `hal/`, and `test/` code.

### Documentation Structure (`docs/`)

*   **`docs/ARCHITECTURE.md` (The Constitution):** Defines system invariants and constraints (e.g., "HAL must never include Arduino.h"). This is the rulebook the Builder must check before coding.
*   **`docs/IMPLEMENTATION_LOG.md` (The Lab Notebook):** Captures "Tribal Knowledge" and the "Why" behind complex technical decisions to prevent regression.
*   **`features/` (The Spec):** The actual requirements. The status of these files drives the development loop.

## 🧪 Testing Strategy

We employ a dual-layer testing strategy to ensure both logic correctness and hardware fidelity.

### 1. Unit Testing (Native)
*   **Purpose:** Verifies business logic, data processing, and UI layout math without needing physical hardware.
*   **Execution:** Runs on the host machine (Mac/Linux/Windows) using the PlatformIO native environment.
*   **Command:**
    ```bash
    pio test -e native_test
    ```

### 2. Hardware-in-Loop (HIL) Testing
*   **Purpose:** Verifies that the code works correctly on the physical ESP32 boards (e.g., display drivers, touch response, WiFi).
*   **Execution:** These are often visual or interactive tests defined in the `## Hardware (HIL) Test` section of a feature file. The Builder implements a temporary demo in `main.cpp` to prove the feature works.
*   **Validation:** Requires human-in-the-loop confirmation.

### 3. CDD Monitor & Test Status
The **Continuous Documentation Dashboard (CDD)** monitors the lifecycle of a feature:
*   **`TODO`**: The spec file (`features/X.md`) has been modified more recently than the last implementation commit.
*   **`TESTING`**: The Builder has implemented the feature and created a commit with the tag `[Ready for HIL Test features/X.md]`. This signals that unit tests pass and it's ready for physical board verification.
*   **`DONE`**: Once verified on hardware (or by passing final integration tests), a commit with the tag `[Complete features/X.md]` marks the feature as finished.

---

## 🛠️ DevOps & Tooling

We have built custom tooling to visualize and manage this workflow.

### 1. CDD Web Monitor (Continuous Documentation Dashboard)
A real-time dashboard that tracks the synchronization between Specs and Code.
*   **Function:** Monitors git commit timestamps. If a Feature File is newer than its implementation commit, it flags the feature as `[TODO]`.
*   **Usage:**
    ```bash
    ./ai_dev_tools/cdd/start.sh
    # Open http://localhost:8086
    ```

### 2. Software Map (Dependency Visualization)
An interactive node graph of the project's feature dependencies.
*   **Function:** Visualizes the hierarchy from `RELEASE` nodes down to specific hardware specs.
*   **Usage:**
    ```bash
    ./ai_dev_tools/software_map/start.sh
    # Open http://localhost:8085
    ```

### Setup
Ensure you have Python 3 installed. The tools use standard libraries or minimal dependencies.
*   **PlatformIO:** Required for building the firmware.
    ```bash
    pip install platformio
    ```

---

## ⚡ Supported Hardware (HAL)

This project uses a strict **Hardware Abstraction Layer (HAL)**. Application code never touches hardware directly.

We currently support two primary boards from LilyGo:

| Board Name | Environment Name | Key Features |
| :--- | :--- | :--- |
| **[Waveshare ESP32-S3 1.8 AMOLED Touch](https://www.waveshare.com/esp32-s3-touch-amoled-1.8.htm?srsltid=AfmBOoqeOA9TgJGfhIbtYCvXR7oEmlO_g-zDU1NZwziZdzl7I1HydyTj)** | `env:esp32s3` | 1.8" AMOLED (SH8601), CST816 Touch |
| **[LilyGo T-Display-S3 AMOLED Plus](https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series?tab=readme-ov-file)** | `env:tdisplay_s3_plus` | 1.91" AMOLED (RM67162), CST816 Touch |

### Build Commands
To build and upload the latest verified release:
```bash
# For AMOLED version
pio run -e demo_v065_esp32s3 -t upload

# For Plus version
pio run -e demo_v065_tdisplay -t upload
```

---

## ✨ Current Features (Firmware)

The firmware is currently at **Milestone v0.65**.

*   **Graph V2 Engine:** High-performance, thread-safe plotting engine with autonomous layout, collision avoidance, and significant-digit aware labeling.
*   **Stock Tracker:** Real-time stock data tracking using the Yahoo Finance API (via WiFi).
*   **Touch Interaction (New in v0.65):** Full gesture engine supporting Tap, Hold, and Edge Drag operations via the CST816 driver.
*   **UI Framework:** Themeable components, MiniLogo vector rendering, and persistent overlays.
*   **Agentic CI/CD:** Automated verification of feature states via the CDD Monitor.

## 📄 License
This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**. See the [LICENSE.md](LICENSE.md) file for details.
