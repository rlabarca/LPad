# Feature: CDD Status Monitor

> Label: "Tool: CDD Monitor"
> Category: "DevOps Tools"
> Prerequisite: agentic_devops/features/arch_agentic_workflow.md

## 1. Overview
The Continuous Deployment-Driven (CDD) Monitor tracks the status of all feature files across both the Application (LPad) and Agentic (DevOps) domains.

## 2. Requirements

### 2.1 Domain Separation
*   **Split View:** The UI must display two distinct columns/sections: "LPad Application" and "Agentic DevOps".
*   **Path Awareness:** The monitor must scan `features/` for LPad status and `agentic_devops/features/` for DevOps status.
*   **Tag Parsing:** Status is derived from git commit tags using the full relative path (e.g., `[Complete features/X.md]` or `[Complete agentic_devops/features/Y.md]`).

### 2.2 UI & Layout
*   **Compact Design:** Minimal padding and margins to ensure the dashboard fits in a small, side-docked browser window.
*   **DONE List Capping:** The "DONE" section for each domain must be limited to the **10 most recent items**. If more exist, display "and X more..." with a count.
*   **Status Indicators:** Maintain the Gold (TODO), Blue (TESTING), and Green (DONE) color coding.

### 2.3 Verification Signals
*   **LPad Tests:** Monitor `.pio/testing/last_summary.json` for firmware logic status.
*   **DevOps Tests:** Monitor a new `agentic_devops/tools/test_summary.json` (if it exists) or verify the success of the last `software_map` generation.

## 3. Scenarios

### Scenario: Domain Isolation
    Given a commit with tag "[Complete agentic_devops/features/tool_x.md]"
    When the CDD monitor refreshes
    Then "tool_x.md" appears in the DONE section of the Agentic DevOps column
    And it does NOT appear in the LPad Application column

## 4. Implementation Notes
*   **Git Efficiency:** Use `git log --grep` with the full relative path to avoid collision between domains.
*   **Visual Polish:** Use a dark, high-contrast theme suitable for 24/7 monitoring.
