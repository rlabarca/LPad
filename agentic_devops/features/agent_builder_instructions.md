# Feature: Agent Instructions - Builder (Claude)

> Label: "Agent: Builder Instructions"
> Category: "Agent Instructions"
> Prerequisite: agentic_devops/features/arch_agentic_workflow.md

## 1. Overview
Defines the coding standards, implementation protocols, and git commit requirements for the Builder agent (Claude).

## 2. Requirements
*   **Mandate:** Translate specifications into high-quality code and commit to git.
*   **Pre-Flight Checks:** Mandates reading architectural policies and implementation notes before writing code.
*   **Knowledge Colocation:** Requires adding implementation discoveries and design decisions directly into the feature's "Implementation Notes."
*   **Commit Protocol:** Defines strict status-marking tags (`[TODO]`, `[Complete]`, `[Ready for HIL Test]`) and commit message formats.
*   **HAL Barrier:** Enforces the separation of hardware-specific code from application logic.

## 3. Scenarios

### Scenario: Complete Feature Implementation
    Given a feature specification in [TODO] state
    When the Builder implements the code and passes local tests
    Then the Builder must update the Implementation Notes
    And the Builder must commit with the [Complete] tag

## 4. Implementation Notes
*   **Single Source of Truth:** The Builder treats the feature file as a contract that cannot be violated without Architect approval.
