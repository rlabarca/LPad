# Feature: CDD Status Monitor

> Label: "Tool: CDD Monitor"
> Category: "DevOps Tools"
> Prerequisite: agentic_devops/features/arch_agentic_workflow.md

## 1. Overview
The Continuous Deployment-Driven (CDD) Monitor tracks the status of all feature files in the system by parsing git commit messages for specific tags.

## 2. Status Tags
*   **[TODO]:** Initial state of a feature or any feature modified after completion.
*   **[Ready for HIL Test]:** Implementation complete, unit tests passed, awaiting hardware verification.
*   **[Complete]:** Feature fully implemented and verified.

## 3. Behavior
*   **Status Tracking:** The monitor scans the git history and correlates the latest tag for each `features/*.md` file.
*   **Dependency Awareness:** Features are marked as "Blocked" if their prerequisites are not `[Complete]`.
*   **Web Interface:** Serves a dashboard showing the state of the feature queue.

## 4. Implementation Notes
*   **Storage:** Status is derived purely from git; there is no sidecar database.
*   **Trigger:** Status resets to `[TODO]` whenever a feature file is modified in a commit without a completion tag.
