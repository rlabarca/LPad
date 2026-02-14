# Agentic Process History

This log tracks the evolution of our DevOps processes and agentic protocols. Entries are used to populate the "Agentic Evolution" table in the root `README.md` before each GitHub push.

## [2026-02-12] v0.70 Agentic DevOps Refactor
*   **Consolidated Home:** Created `agentic_devops/` as the central hub for the workflow.
*   **Architectural Inquiry:** Mandated proactive questioning to improve specification clarity and architectural integrity in `ARCHITECT_INSTRUCTIONS.md`.
*   **Release Protocol:** Codified a 6-step Release Protocol (Audit, Cleanup, Sync, Push) in `ARCHITECT_INSTRUCTIONS.md`.
*   **Cleanup Tool:** Created `cleanup_orphaned_features.py` to maintain feature-tree integrity.
*   **Drift Remediation:** Updated `ARCHITECT_INSTRUCTIONS.md` to allow direct Builder remediation of architectural drift without new feature files.
*   **Living Specs:** Formalized the "No v2 Files" rule; features are updated in-place.
*   **Knowledge Colocation:** Migrated global implementation log into individual feature files.
*   **Modular Architecture:** Replaced monolithic `ARCHITECTURE.md` with granular `arch_*.md` policy nodes in the dependency graph.
*   **Instruction Refactoring:** Separated "Theory" (`HOW_WE_WORK.md`) from "Practice" (`*_INSTRUCTIONS.md`).
*   **Evolution Tracking:** Codified the requirement to track process changes in parallel with software releases.

## [2026-02-13] Milestone: Release v0.70 - UI Render Manager
*   **Architectural Shift:** Migrated from monolithic "Demo Apps" to a managed `UIRenderManager` with Z-order compositing and input routing.
*   **Role Maturity:** Successfully executed the first release under the new "Architect/Builder" role separation.
*   **HIL Success:** Verified multi-layer rendering and system-level gesture interception on physical hardware.
*   **Tree Integrity:** Re-anchored all HAL and Infrastructure specifications to policy nodes, resolving dependency orphans.
*   **Style Mandate:** Formalized the "No Emojis" rule for all Markdown documentation in `ARCHITECT_INSTRUCTIONS.md`.
*   **Screenshot Protocol:** Defined requirements for a Serial Screenshot Utility with an executable shell wrapper and automated `.venv` activation.
*   **Acyclic Mandate:** Formalized the requirement to avoid and verify circular dependencies in `ARCHITECT_INSTRUCTIONS.md` following a graph recursion error.
*   **Test Fidelity Mandate:** Codified the requirement in `ARCHITECT_INSTRUCTIONS.md` to maintain explicit, high-rigor HIL testing steps in all release specifications to prevent regression drift.

## [2026-02-13] Milestone: Release v0.71 - Serial Screenshot Utility
- **Feature Capability:** Implemented a Serial-triggered screenshot tool that dumps the display shadow buffer from PSRAM to a host PNG.
- **Developer Utility:** Introduced \`scripts/screenshot.sh\` as a cross-platform capture tool with automated \`.venv\` management.
- **Architectural Cleanup:** Resolved circular dependencies in the core architecture and established the acyclic graph mandate.
- **Documentation Standard:** Formalized the "No Emojis" mandate for all Markdown documentation.

## [2026-02-14] Meta-Process Evolution: Agentic DevOps Self-Specification
- **Recursive Governance:** Applied the Spec-Driven Workflow to the `agentic_devops` framework itself.
- **Domain Separation:** Established `agentic_devops/features/` for meta-specifications, separating them from the `features/` application domain.
- **Bootstrap Specs:** Authored initial meta-features for `arch_agentic_workflow`, `cdd_status_monitor`, `software_map_generator`, and `proc_history_management`.
- **Granular Instruction Tracking:** Created dedicated feature specifications for `agent_architect_instructions.md` and `agent_builder_instructions.md` under a new "Agent Instructions" category to manage role evolutions.
- **Instruction Refinement:** Updated `ARCHITECT_INSTRUCTIONS.md` and `BUILDER_INSTRUCTIONS.md` to recognize and support the dual-domain specification architecture.
- **Documentation Sync:** Updated `HOW_WE_WORK.md` and the root `README.md` to reflect the new separation of concerns.
- **UI Refinement:** Fixed a layout/zoom race condition in the Software Map Browser and added multi-tab support for domain-specific dependency trees (LPad vs. Agentic DevOps).
- **Agentic Orchestration:** Empowered the Builder agent to orchestrate specialized sub-agents and internal personas for complex tasks to improve implementation accuracy.
- **Test Isolation Mandate:** Enforced strict separation of test execution between LPad (PlatformIO/C++) and Agentic (Python/Tools) domains. Builders must now use domain-local test runners for DevOps tools to avoid polluting the firmware test logs.
- **DevOps Test Protocol:** Standardized tool-level test reporting via `test_status.json`. The CDD monitor now aggregates these files to provide a real-time "Agentic Test Status."
- **Synchronized Release Protocol:** Established a dual-domain audit requirement for GitHub pushes, ensuring firmware stability and DevOps toolchain health are validated in parallel.

## [2026-02-13] Milestone: Release v0.72 - UI Widgets & Multi-WiFi
- **Widget Architecture:** Implemented a formal `UI Widget System` using relative positioning and layout heuristics.
- **UI Framework Expansion:** Delivered `WidgetLayoutEngine`, `GridWidgetLayout`, `TextWidget`, and `ScrollableListWidget`.
- **Network Evolution:** Updated HAL and Config to support multiple APs with iterative boot-time fallback and manual runtime selection.
- **HIL Success:** Verified "window shade" animation synchronization, blinking connection states, and fallback SSID persistence on physical hardware.
- **UI Refinement:** Iteratively tuned the "Section" heading style (12pt, underlined, Khaki) and layout justification based on HIL feedback.


