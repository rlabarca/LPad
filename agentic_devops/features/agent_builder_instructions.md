# Feature: Agent Instructions - Builder (Claude)

> Label: "Agent: Builder Instructions"
> Category: "Agent Instructions"
> Prerequisite: agentic_devops/features/arch_agentic_workflow.md

## 1. Overview
Defines the implementation protocols and domain-aware requirements for the Builder agent.

## 2. Requirements

### 2.1 Domain-Aware Implementation
*   **LPad Context (`features/`):** 
    *   Targets: `src/`, `hal/`, `include/`.
    *   Tests: MUST be placed in `test/`.
    *   Tooling: Use PlatformIO (`pio test`).
*   **DevOps Context (`agentic_devops/features/`):**
    *   Targets: `agentic_devops/tools/`.
    *   Tests: MUST be colocated within the tool's directory (e.g., `agentic_devops/tools/cdd/tests/`) or as standalone scripts.
    *   Constraint: NEVER place DevOps/Process tests in the project's root `test/` folder.

### 2.2 Pre-Flight Checks
*   **Consult the Architecture:** Identify if the task is "Application" or "Agentic" and read the corresponding `arch_*.md`.
*   **Knowledge Base:** Read `## Implementation Notes` in the target file and its prerequisites.

### 2.3 Commit & Status Protocol
*   **Tag Format:** Must include the full path: `[Complete <path_to_feature>]`.
*   **Tag Mapping:** 
    *   `[TODO]`: Active work.
    *   `[Ready for HIL Test]`: Firmware ready for hardware.
    *   `[Complete]`: Logic verified (for tools) or HIL passed (for firmware).

### 2.5 Domain-Specific Test Execution
*   **Command Isolation:** The Builder MUST NOT use the global `./scripts/test_local.sh` or `pio test` when working in the Agentic domain.
*   **Local Test Runners:** The Builder must look for and execute test scripts located within the specific tool's directory (e.g., `agentic_devops/tools/cdd/run_tests.sh`).
*   **No Pollution:** Running Agentic tests must never trigger a re-compilation or execution of LPad firmware tests.

## 3. Scenarios

### Scenario: Implementing a DevOps Tool
    Given a feature in `agentic_devops/features/`
    When the Builder implements the script
    Then all code must stay within `agentic_devops/tools/`
    And any tests must be internal to the agentic devops directory

### Scenario: Complex System Refactoring
    Given a request to refactor a core HAL interface
    When the Builder identifies high risk of regression
    Then the Builder should delegate a deep dependency analysis to the `codebase_investigator`
    And the Builder should use a "Critic" persona to validate the new interface against the HAL policy

## 4. Implementation Notes
*   **Context Isolation:** The Builder must maintain a strict mental firewall between the embedded firmware world and the Python-based DevOps world.
