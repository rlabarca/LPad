# Agentic Protocols for Rich (CTO/Robotics Profile)

## Executive Summary & Core Mandate
Your primary function is to translate `features/*.md` files into code and then **commit that code to git**. The git commit is the *only* way to mark your work as complete. If you do not create the commit, from the system's perspective, you have done nothing. **Code without a commit is not considered done.**

---

## The Core Workflow: Your Single Most Important Task
This is the required sequence of actions for every feature you implement. You MUST follow these steps exactly.

**Step 1: Understand the Feature**
- Read the `features/*.md` file you have been assigned.
- Pay attention to the `Prerequisite:` to understand your available tools.

**Step 2: Implement the Code**
- Write the code and corresponding unit tests to satisfy the feature's scenarios.
- Follow all architectural rules (HAL, TDD, etc.).

**Step 3: Finalize and Commit (The 'Definition of Done')**
Once the code is written and unit tests pass, you are not finished. You MUST conclude your work by performing the following shell commands:

1.  **Stage your changes:**
    ```shell
    git add .
    ```

2.  **Commit your work with a status-marking message:**
    *   **If the feature has a `## Hardware (HIL) Test` section:**
        ```shell
        git commit -m "feat(WIP): Implementation complete, ready for HIL test [Ready for HIL Test features/your_feature_file.md]"
        ```
        *(Replace `your_feature_file.md` with the actual filename.)*

    *   **If the feature does NOT have a HIL test section:**
        ```shell
        git commit -m "feat(done): Implementation complete and validated [Complete features/your_feature_file.md]"
        ```
        *(Replace `your_feature_file.md` with the actual filename.)*

3.  **Handoff to User:**
    *   After the commit is successful, inform the user that "The feature `features/your_feature_file.md` is now `[Ready for HIL Test]` and awaiting user validation." or that it is `[Complete]`.

**Step 4: Await HIL Test Confirmation (If Applicable)**
- If the feature is `[Ready for HIL Test]`, do not do anything else until the user confirms the HIL test has passed.
- Once the user confirms, create the final "complete" commit:
    ```shell
    git commit --allow-empty -m "chore(release): HIL validation passed [Complete features/your_feature_file.md]"
    ```

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
