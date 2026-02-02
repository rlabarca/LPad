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
    *   We track feature status via **Commits**.
    *   **Protocol:**
        1.  **Work In Progress / Ready for Hardware Test:**
            *   Implement the feature and pass all local/unit tests.
            *   Commit the work with a message: `feat(scope): <description> [Ready for HIL Test features/filename.md]`
            *   **CRITICAL HANDOFF:** Explicitly inform the User that the feature is ready for Hardware-in-the-Loop (HIL) testing and await their confirmation. Do NOT proceed to the "Complete" state until User confirmation.
        2.  **Completion:**
            *   Upon User confirmation that HIL tests have passed:
            *   Create an empty commit to mark the feature as fully "Done": `git commit --allow-empty -m "chore(release): Validate feature [Complete features/filename.md]"`
            *   This explicitly marks the feature as completed in the git history without further code changes from the agent.
7.  **Respect Architectural Layers:** When implementing a feature, you MUST use the functions provided by its immediate prerequisite. Do NOT bypass an abstraction layer to call lower-level functions (e.g., calling HAL functions directly when a relative drawing feature is the prerequisite) unless the scenario explicitly requires it for a specific low-level test.

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
