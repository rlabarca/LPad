# Agentic Protocols for Rich (CTO/Robotics Profile)

## Project Context
> **CRITICAL:** We do NOT use external task trackers (Beads/Jira) or Claude's native `/task` command (which is user-bound). The **Git History** and **Filesystem** are the only Source of Truth.

## Core Directives
1.  **Feature-First:** Code is secondary. Truth lives in `features/*.md`. Strictly follow the Gherkin-style behavior defined there.
2.  **HAL Architecture:** NEVER write hardware-specific code in `src/`. All hardware interaction (GPIO, I2C, Display) must go through `hal/`.
3.  **TDD Workflow:**
    * Write the test (Unity), see it fail, write the code, pass.
    * **Agent Behavior:** If you lose context, run `git log --oneline` or `ls features/` to re-orient yourself.
4.  **Token Efficiency:**
    * Do NOT try to flash/monitor the device directly. Ask Rich to "Run target X".
5.  **Tooling Constraints:**
    * **Editor:** I use Helix (`hx`).
    * **Version Control:** Local git only. No pushing to remote.
6.  **Version Control (The State of Truth):**
    * We track "Done" status via **Commits**.
    * **Protocol:** When a Feature is tested and ready:
        1.  Run `scripts/test_local.sh` (or `pio run`) to ensure green state.
        2.  Run `git status`.
        3.  Run `git add -A`.
        4.  **CRITICAL:** The commit message MUST reference the feature file to mark it "Done".
            * Format: `feat(scope): <description> [Complete features/filename.md]`

## Architecture & Discovery
* This is a PlatformIO project.
* Inspect `platformio.ini` to understand build targets and library dependencies.

## Hardware Implementation Protocol
*   When a feature requires implementing a HAL for a specific piece of hardware, it MUST begin by consulting the `hw-examples/` directory.
*   Locate the sub-directory corresponding to the target hardware (e.g., `hw-examples/ESP32-S3-Touch-AMOLED-1.8-Demo/`).
*   The code you write must be a direct port of the logic and initialization sequences found in the vendor-provided examples within that directory. The feature file will specify the exact reference path.

## Dependency & Workflow Logic
We use a **Distributed Dependency Graph** embedded in Feature Files to determine the order of work.

1.  **Source of Truth:** The state of the project is defined by the files in `features/`.
2.  **Dependency Header:** Every feature file begins with `> Prerequisite: [Feature Name | None]`.
3.  **Task Selection Algorithm:**
    To determine the next task, you must:
    a.  **Scan** all `features/*.md` files.
    b.  **Check History** (`git log`) to identify which features are already **Completed** (grep for "Complete features/...").
    c.  **Select** a feature where:
        * The `Prerequisite` is "None" **OR** the `Prerequisite` is confirmed "Completed" in the logs.
        * AND the feature itself is **NOT** "Completed".
4.  **Execution:**
    * Announce the start of the feature.
    * Implement the Scenarios using TDD.
    * **Commit** using the protocol in Core Directive #6 to "Close" the task.
