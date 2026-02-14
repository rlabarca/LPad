# Feature: Dual-Domain Release Protocol

> Label: "Proc: Release Protocol"
> Category: "Process"
> Prerequisite: agentic_devops/features/arch_agentic_workflow.md

## 1. Overview
The Release Protocol is a synchronized system audit that ensures both the Application (LPad) and the Meta-System (Agentic DevOps) are verified, documented, and stable before any push to the remote repository.

## 2. Requirements

### 2.1 Verification Gates
*   **LPad Gate:** MUST verify a PASS status from PlatformIO native tests.
*   **DevOps Gate:** MUST verify a PASS status from the aggregated `test_status.json` files in `agentic_devops/tools/`.
*   **Partial Releases:** If a domain (Application or Agentic) has no functional changes, it is marked as **[STABLE]** in the evolution table. It MUST still pass its verification gate as a regression check.
*   **Blocker:** The Architect is forbidden from initiating a push if either domain reports a failure.

### 2.2 Synchronization Mandates
*   **Software Map:** MUST regenerate both LPad and Agentic DevOps feature maps. The Architect must verify the absence of orphans or circular dependencies in both domains.
*   **Evolution Parity:** The root `README.md` must be updated to reflect the parallel milestones of Firmware (Application) and Agentic DevOps (Process).
*   **Instruction Audit:** The Architect must verify that the operational instruction files (`*_INSTRUCTIONS.md`) match the latest behavioral specifications in `agentic_devops/features/`.

### 2.3 Cleanup
*   **Orphan Staging:** Execute `cleanup_orphaned_features.py` to identify and move deprecated specs to `.trash/` after user approval.

## 3. Scenarios

### Scenario: Synchronized Release
    Given LPad features are [Complete]
    And Agentic features are [Complete]
    And all tests are passing in both domains
    When the user issues a release command
    Then the Architect regenerates both software maps
    And the Architect syncs the Evolution Table in README.md
    And the Architect performs a final git status check before commit

## 4. Implementation Notes
*   **Deterministic Integrity:** This protocol prevents "Process Drift" where the workflow instructions become disconnected from the actual tools in use.
