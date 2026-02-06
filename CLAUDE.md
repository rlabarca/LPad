# Agentic Protocols for Rich (CTO/Robotics Profile)

## Executive Summary & Core Mandate
Your primary function is to translate `features/*.md` files into code and then **commit that code to git**. The git commit is the *only* way to mark your work as complete. If you do not create the commit, from the system's perspective, you have done nothing. **Code without a commit is not considered done.**

---

## The Core Workflow: Your Unbreakable Contract

This is the required sequence of actions for every feature you implement. This is not a suggestion; it is a contract. You MUST follow these steps exactly.

**Step 0: Mandatory Pre-computation (The "Prime Directive")**
Before writing any code, you MUST consult the project's core documents. This is not optional.
1.  **Consult `docs/ARCHITECTURE.md` (The Constitution):** You must read this file to understand the non-negotiable rules of the system. Your implementation must not violate these rules.
2.  **Consult `docs/IMPLEMENTATION_LOG.md` (The Lab Notebook):** You must read this file to learn from past decisions and avoid repeating documented mistakes.

**Step 1: Understand & Research**
- **Read the Spec:** Read the `features/*.md` file you have been assigned.
- **Identify Dependencies:** Identify all prerequisite features and available HAL abstractions.

**Step 2: Recursive Dependency Check (Crucial)**
- **Check Status:** Before implementing the target feature, check the git log for its prerequisites.
- **The "Chain Reaction" Rule:** If a prerequisite feature is currently `[TODO]` (e.g., due to a documentation update), you **MUST** verify and commit the prerequisite *before* (or in the same turn as) the target feature.
- **Verification Sweeps:** You are authorized to create multiple `feat(verify)` commits in a single turn to clear a chain of outdated dependencies. Work from the bottom up (leaves first).

**Step 3: Implement and Test**
- Write the code and corresponding unit tests to satisfy the feature's scenarios.
- Run the unit tests using `./scripts/test_local.sh`. This script updates the `.pio/testing/last_summary.json` file used by the CDD monitor.
- Ensure all tests pass before proceeding.

**Step 4: Document Lessons Learned**
- **The "Lab Notebook" Rule:** If you discovered a non-obvious solution, overcame a significant technical challenge, or made a key decision (e.g., "why I chose algorithm X over Y"), you **MUST** append a concise entry to `docs/IMPLEMENTATION_LOG.md`.
- **Format:** Use Markdown, starting with a `### <Date>` and `#### <Feature: features/your_feature.md>` heading.
- **Purpose:** This prevents future agents from repeating your work and justifies your design choices.

**Step 5: THE COMMIT PROTOCOL (Your Final Action)**
This is the **most critical step**. The monitoring script (`cdd.sh`) is a **DUMB SCRIPT**. It looks for **EXACT STRING MATCHES**. It cannot understand "creative" variations.

**Your final action for ANY task MUST be to execute these two shell commands in order:**

1.  **Stage all your changes:**
    ```shell
    git add .
    ```

2.  **Create the status-marking commit:**

    **CRITICAL: You MUST check the feature file to determine the correct commit status tag.**

    *   **If the feature requires hardware validation, use `[Ready for HIL Test features/FILENAME.md]`:**

        A feature requires hardware validation if it has ANY of these:
        - A `## Hardware (HIL) Test` section
        - An `## Integration Test Criteria` section that mentions "target hardware" or "on the device"
        - Is a demo/app feature (in Category: "Release Demos" or "Applications")
        - Explicitly states it must be "validated on hardware" anywhere in the spec

        **Format:**
        ```shell
        git commit -m "feat(WIP): <Brief Description> [Ready for HIL Test features/FILENAME.md]"
        ```
        *(Example: `git commit -m "feat(WIP): Implemented bonding logic [Ready for HIL Test features/app_bond_tracker.md]"}`)*

    *   **If the feature does NOT require hardware validation, use `[Complete features/FILENAME.md]`:**

        Use this ONLY for pure software features that can be fully validated with unit tests:
        - Library/utility code (Category: "Core", "Utilities", "HAL Contracts")
        - Features with only software unit test scenarios
        - No hardware dependencies mentioned in the spec

        **Format:**
        ```shell
        git commit -m "feat(done): <Brief Description> [Complete features/FILENAME.md]"
        ```
        *(Example: `git commit -m "feat(done): Added helper functions [Complete features/hal_contracts.md]"}`)*

    *   **SPECIAL CASE - Verification-only commits (no code changes):**

        If you are re-validating an existing feature and `git commit` fails with "nothing to commit", use `--allow-empty`:
        ```shell
        git commit --allow-empty -m "feat(verify): <Brief Description> [Ready for HIL Test features/FILENAME.md]"
        ```
        OR
        ```shell
        git commit --allow-empty -m "feat(verify): <Brief Description> [Complete features/FILENAME.md]"
        ```
        The choice between `[Ready for HIL Test]` vs `[Complete]` follows the same rules above.

### Special Case: Re-Validation & Status Reset

The `cdd.sh` script monitors feature status based on git commit timestamps. **Any modification to a feature file (`features/X.md`) automatically resets its status to `[TODO]`, even if the underlying code remains unchanged.**

*   **Your Action:** If you are asked to "verify" or "re-implement" a feature (e.g., after an architectural change like a feature file rename or label update), you **MUST** ensure the corresponding commit uses the `[Complete features/FILENAME.md]` or `[Ready for HIL Test features/FILENAME.md]` tag.
*   **No Code Changes:** If tests pass without requiring any code modifications, use the `--allow-empty` flag with your commit to explicitly record the verification timestamp.
    ```shell
    git commit --allow-empty -m "feat(verify): Re-validated feature after architectural changes [Complete features/FILENAME.md]"
    ```
    This ensures the `cdd.sh` script correctly updates the feature's status to `DONE` or `TESTING`.

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
    d. **Proactive Status Re-evaluation:** After any modification to `features/`, `README.md`, or `feature_graph.mmd` (e.g., due to architectural refactoring, renames, label updates), you **MUST** re-scan all `features/*.md` files and re-evaluate their `TODO` status based on the `cdd.sh` monitoring script's logic. Any feature newly flagged as `[TODO]` (due to its file's timestamp being newer than its last `[Complete]` or `[Ready for HIL Test]` commit) must then be prioritized for verification according to Step 2 ("Recursive Dependency Check").