# Agentic Protocols for Rich (CTO/Robotics Profile)

## Executive Summary & Core Mandate
Your primary function is to translate `features/*.md` files into code and then **commit that code to git**. The git commit is the *only* way to mark your work as complete. If you do not create the commit, from the system's perspective, you have done nothing. **Code without a commit is not considered done.**

---

## The Core Workflow: Your Unbreakable Contract

This is the required sequence of actions for every feature you implement. This is not a suggestion; it is a contract. You MUST follow these steps exactly.

**Step 1: Understand the Feature**
- Read the `features/*.md` file you have been assigned.
- Identify all prerequisite features and available HAL abstractions.

**Step 2: Implement and Test**
- Write the code and corresponding unit tests to satisfy the feature's scenarios.
- Run the unit tests and ensure they pass.

**Step 3: THE COMMIT PROTOCOL (Your Final Action)**
This is the **most critical step**. After your code is written and tested, your work is **not done**. You MUST finalize your work by creating a git commit. This is your ONLY output.

**Your final action for ANY task MUST be to execute these two shell commands in order:**

1.  **Stage all your changes:**
    ```shell
    git add .
    ```

2.  **Create the status-marking commit:**
    *   **If the feature has a `## Hardware (HIL) Test` section:** Use this exact commit message format, replacing only the filename:
        ```shell
        git commit -m "feat(WIP): Implementation complete, ready for HIL test [Ready for HIL Test features/the_feature_file.md]"
        ```
        *(Example: `git commit -m "feat(WIP): Implementation complete, ready for HIL test [Ready for HIL Test features/app_bond_tracker.md]"}`)*

    *   **If the feature does NOT have a HIL test section:** Use this exact commit message format, replacing only the filename:
        ```shell
        git commit -m "feat(done): Implementation complete and validated [Complete features/the_feature_file.md]"
        ```
        *(Example: `git commit -m "feat(done): Implementation complete and validated [Complete features/hal_contracts.md]"}`)*

**DO NOT** add any extra conversation or explanation after the commit. The commit IS your final handoff. If you have failed to commit, you have failed the task.

**Self-Correction:** If the user asks if your work is committed, and it is not, you must immediately apologize and execute the COMMIT PROTOCOL.

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
