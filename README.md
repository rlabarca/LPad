# <img src="assets/LPadLogo.svg" width="48" height="48" valign="middle"> LPad

An autonomous ecosystem of tools to help me run my life, starting with embedded systems for financial data visualization.

![Release v0.5](https://img.shields.io/badge/Release-v0.5-orange) ![Status](https://img.shields.io/badge/Status-Active-green)

## 🛠️ Development Environment (macOS)

To build and extend this project, you need the following tools installed on your system.

### Base Dependencies (Homebrew)
```bash
# PlatformIO Core (Build System)
brew install platformio

# Node.js (Required for Mermaid CLI)
brew install node

# Git & Python3 (Standard)
brew install git python3
```

### Visual Tooling (NPM)
We use `mermaid-cli` to generate architecture diagrams directly in the terminal.
```bash
# Mermaid CLI
npm install -g @mermaid-js/mermaid-cli
```

### IDE Support
*   **VS Code** with the **PlatformIO IDE** extension is highly recommended.
*   **iTerm2** is recommended for terminal usage to support inline image rendering of graphs.

---

## 🧠 The Agentic Workflow

This project is not built by humans writing C++. It is constructed using a rigorous **Agentic Workflow** where:

1.  **The Architect (User/Gemini):** Defines *what* the system does using Gherkin-style Feature Files (`features/*.md`) and maintains the System Constitution (`docs/ARCHITECTURE.md`).
2.  **The Builder (Claude):** Reads the specs and writes the code to satisfy them, committing strictly when tests pass.
3.  **The Process (Scripts):** Automated scripts (`cdd.sh`) monitor file timestamps to enforce a "Stale Spec = Broken Build" philosophy.

### Core Principles
*   **Feature-First:** The `features/` directory is the Source of Truth.
*   **Code is Disposable:** `src/` can be deleted and rebuilt entirely from the features.
*   **Layered Architecture:** Strict separation between Hardware Abstraction Layer (HAL) and Application Logic.

---

### Agent Configuration

This project includes a `.gemini/settings.json` file. This file provides project-specific configuration for the Gemini CLI agent, which plays the "Architect" role.

-   **`context.fileName`**: This setting automatically loads the `GEMINI_ARCHITECT.md` file into the agent's context at the start of a session. This ensures the "Architect" agent is always aware of its core mission, responsibilities, and the project's development philosophy without needing to be manually prompted.

By checking this file into the repository, we ensure that any developer (or agent) using this project will have their tools configured consistently.

---

## 🏗️ Project Architecture

### Dependency Graph
The following DAG (Directed Acyclic Graph) represents the current feature set and their dependencies. This is generated automatically from the `features/` directory.

<!-- MERMAID_START -->
```mermaid
graph TD
    %% Styling
    classDef default fill:#e1f5fe,stroke:#01579b,stroke-width:1px,color:black;
    classDef release fill:#f96,stroke:#333,stroke-width:2px,color:black,font-weight:bold;
    classDef hardware fill:#e8f5e9,stroke:#2e7d32,stroke-width:1px,color:black;
    classDef ui fill:#f3e5f5,stroke:#7b1fa2,stroke-width:1px,color:black;
    classDef app fill:#fff3e0,stroke:#e65100,stroke-width:1px,color:black;
    classDef graphics fill:#e0f7fa,stroke:#006064,stroke-width:1px,color:black;

    subgraph Application_Layer ["Application Layer"]
        direction TB
        app_animation_ticker("**Animation Ticker Engine**<br/><small>app_animation_ticker.md</small>"):::app
    end

    subgraph Board_Drivers ["Board Drivers"]
        direction TB
        display_esp32s3_amoled("**ESP32-S3 AMOLED Driver**<br/><small>display_esp32s3_amoled.md</small>")
        display_tdisplay_s3_plus("**T-Display S3+ Driver**<br/><small>display_tdisplay_s3_plus.md</small>")
    end

    subgraph Graphics_Engine ["Graphics Engine"]
        direction TB
        display_canvas_drawing("**Layered Canvas Drawing**<br/><small>display_canvas_drawing.md</small>"):::graphics
        display_relative_drawing("**Relative Drawing**<br/><small>display_relative_drawing.md</small>"):::graphics
    end

    subgraph Hardware_Layer ["Hardware Layer"]
        direction TB
        display_rotation_contract("**Rotation Contract**<br/><small>display_rotation_contract.md</small>"):::hardware
        display_target_rotation("**Target Rotation**<br/><small>display_target_rotation.md</small>"):::hardware
        hal_contracts("**HAL Display & Timer Contracts**<br/><small>hal_contracts.md</small>"):::hardware
        hal_dma_blitting("**DMA Blitting**<br/><small>hal_dma_blitting.md</small>"):::hardware
        hal_timer_esp32("**ESP32 Timer**<br/><small>hal_timer_esp32.md</small>"):::hardware
    end

    subgraph Releases ["Releases"]
        direction TB
        RELEASE_v0_5_display_drawing_ui_base("**Release v0.5 - Display, Drawing & UI Base**<br/><small>RELEASE_v0.5_display_drawing_ui_base.md</small>"):::release
    end

    subgraph UI_Framework ["UI Framework"]
        direction TB
        ui_base("**Base UI Elements**<br/><small>ui_base.md</small>"):::ui
        ui_live_indicator("**Animated Live Indicator**<br/><small>ui_live_indicator.md</small>"):::ui
        ui_theme_support("**Theme Support**<br/><small>ui_theme_support.md</small>"):::ui
        ui_themeable_time_series_graph("**Themeable Graph**<br/><small>ui_themeable_time_series_graph.md</small>"):::ui
    end

    %% Relationships
    hal_contracts --> RELEASE_v0_5_display_drawing_ui_base
    hal_timer_esp32 --> RELEASE_v0_5_display_drawing_ui_base
    hal_dma_blitting --> RELEASE_v0_5_display_drawing_ui_base
    display_rotation_contract --> RELEASE_v0_5_display_drawing_ui_base
    display_target_rotation --> RELEASE_v0_5_display_drawing_ui_base
    display_esp32s3_amoled --> RELEASE_v0_5_display_drawing_ui_base
    display_tdisplay_s3_plus --> RELEASE_v0_5_display_drawing_ui_base
    display_canvas_drawing --> RELEASE_v0_5_display_drawing_ui_base
    display_relative_drawing --> RELEASE_v0_5_display_drawing_ui_base
    ui_base --> RELEASE_v0_5_display_drawing_ui_base
    app_animation_ticker --> RELEASE_v0_5_display_drawing_ui_base
    ui_live_indicator --> RELEASE_v0_5_display_drawing_ui_base
    demos/demo_release_0_5[demos/demo_release_0_5?] -.-> RELEASE_v0_5_display_drawing_ui_base
    hal_contracts --> app_animation_ticker
    hal_contracts --> display_canvas_drawing
    hal_contracts --> display_tdisplay_s3_plus
    hal_contracts --> hal_timer_esp32
    display_canvas_drawing --> ui_theme_support
```
<!-- MERMAID_END -->

### Directory Structure

| Directory | Role |
| :--- | :--- |
| `features/` | **Source of Truth.** Gherkin-style specifications. |
| `src/` | **Application Logic.** High-level code (e.g., Graph drawing, Data parsing). |
| `hal/` | **Hardware Abstraction.** Drivers for Display, Timer, etc. |
| `docs/` | **Knowledge Base.** Architecture rules (`ARCHITECTURE.md`) and Implementation Log (`IMPLEMENTATION_LOG.md`). |
| `scripts/` | **Tooling.** `cdd.sh` (Status Monitor) and `generate_graph.sh` (Visualization). |

---

## 🚀 Getting Started

1.  **Environment:** PlatformIO + VS Code.
2.  **Monitor Status:** Run `./scripts/cdd.sh` to see the current development status.
3.  **Build:** `pio run` to compile.
4.  **Upload:** `pio run -t upload` to flash the target.

## 📜 Documentation

*   [System Constitution](docs/ARCHITECTURE.md) - The rules of the road.
*   [Implementation Log](docs/IMPLEMENTATION_LOG.md) - Lessons learned and technical decisions.
*   [Agent Instructions](CLAUDE.md) - The prompt context for the Builder agent.