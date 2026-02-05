# Agentic Protocols for Rich (CTO/Robotics Profile)

## Executive Summary & Core Mandate
Your primary function is to translate `features/*.md` files into code and then **commit that code to git**. The git commit is the *only* way to mark your work as complete. If you do not create the commit, from the system's perspective, you have done nothing. **Code without a commit is not considered done.**

---

## The Core Workflow: Your Unbreakable Contract

This is the required sequence of actions for every feature you implement. This is not a suggestion; it is a contract. You MUST follow these steps exactly.

**Step 1: Understand & Research**
- **Read the Spec:** Read the `features/*.md` file you have been assigned.
- **Read the Constitution:** Read `docs/ARCHITECTURE.md`. You MUST adhere to the constraints defined there (e.g., HAL separation, Data Flow).
- **Read the History:** Read `docs/IMPLEMENTATION_LOG.md`. Check if this problem has been solved (or failed) before. Do not repeat documented mistakes.
- **Identify Dependencies:** Identify all prerequisite features and available HAL abstractions.

**Step 2: Implement and Test**
- Write the code and corresponding unit tests to satisfy the feature's scenarios.
- Run the unit tests and ensure they pass.
- *Self-Correction:* If you encounter a complex technical hurdle and solve it, append a brief entry to `docs/IMPLEMENTATION_LOG.md` explaining the problem and your solution.

**Step 3: THE COMMIT PROTOCOL (Your Final Action)**
This is the **most critical step**. The monitoring script (`cdd.sh`) is a **DUMB SCRIPT**. It looks for **EXACT STRING MATCHES**. It cannot understand "creative" variations.

**Your final action for ANY task MUST be to execute these two shell commands in order:**

1.  **Stage all your changes:**
    ```shell
    git add .
    ```

2.  **Create the status-marking commit:**
    *   **If the feature has a `## Hardware (HIL) Test` section:** Use this EXACT format. Do not change the text inside the brackets:
        ```shell
        git commit -m "feat(WIP): <Brief Description> [Ready for HIL Test features/FILENAME.md]"
        ```
        *(Example: `git commit -m "feat(WIP): Implemented bonding logic [Ready for HIL Test features/app_bond_tracker.md]"}`)*

    *   **If the feature does NOT have a HIL test section:** Use this EXACT format. Do not change the text inside the brackets:
        ```shell
        git commit -m "feat(done): <Brief Description> [Complete features/FILENAME.md]"
        ```
        *(Example: `git commit -m "feat(done): Added helper functions [Complete features/hal_contracts.md]"}`)*

**DO NOT** add any extra conversation or explanation after the commit. The commit IS your final handoff. If you have failed to commit, you have failed the task.

---
## General Directives

1.  **Feature-First:** Code is secondary. Truth lives in `features/*.md`. Strictly follow the Gherkin-style behavior defined there.
2.  **HAL Architecture:** NEVER write hardware-specific code in `src/`. All hardware interaction (GPIO, I2C, Display) must go through `hal/`.
3.  **TDD Workflow:** Write the test (Unity), see it fail, write the code, pass.
4.  **Token Efficiency:** Do NOT try to flash/monitor the device directly. Ask Rich to "Run target X".
5.  **Tooling Constraints:** The editor is Helix (`hx`). Version control is local git only (no pushing to remote).
6.  **Respect Architectural Layers:** When implementing a feature, you MUST use the functions provided by its immediate prerequisite. Do NOT bypass an abstraction layer.

## Architecture & Discovery
* This is a PlatformIO project.
* Inspect `platformio.ini` to understand build targets and library dependencies.

## Hardware Implementation Protocol
*   When a feature requires implementing a HAL for a specific piece of hardware, it MUST begin by consulting the `hw-examples/` directory.
*   Locate the sub-directory corresponding to the target hardware (e.g., `hw-examples/ESP32-S3-Touch-AMOLED-1.8-Demo/`).
*   The code you write must be a direct port of the logic and initialization sequences found in the vendor-provided examples within that directory. The feature file will specify the exact reference path.

## Dependency & Workflow Logic
We use a **Distributed Dependency Graph** embedded in Feature Files to determine the order of work. To determine the next task, you must:
a.  **Scan** all `features/*.md` files.
b.  **Check History** (`git log`) to identify which features are already **Completed**.
c.  **Select** a feature where the `Prerequisite` is completed (or "None") AND the feature itself is not yet completed.