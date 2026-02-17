# Agentic Process History

This log tracks the evolution of our DevOps processes and agentic protocols. Entries are used to populate the "Agentic Evolution" table in the root `README.md` before each GitHub push.

## [2026-02-16] v0.75 CDD Monitor Enhancement
- **Filesystem-Aware Status:** Modified the CDD Status Monitor (`cdd/serve.py`) to use filesystem modification timestamps instead of Git commit history for status detection. This ensures that uncommitted local changes to feature files are immediately reflected, correctly setting their status to `[TODO]`.

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
*   **Role Maturity:** Successfully executed the first release under the new "Architect/Builder" role separation.

## [2026-02-13] v0.71 Process Rigor & Standards
- **Acyclic Mandate:** Formalized the requirement to avoid and verify circular dependencies in `ARCHITECT_INSTRUCTIONS.md` following a graph recursion error.
- **Test Fidelity Mandate:** Codified the requirement in `ARCHITECT_INSTRUCTIONS.md` to maintain explicit, high-rigor HIL testing steps in all release specifications to prevent regression drift.
- **Documentation Standard:** Formalized the "No Emojis" mandate for all Markdown documentation.
- **Screenshot Protocol:** Defined requirements for a Serial Screenshot Utility with an executable shell wrapper and automated `.venv` activation.
- **Milestone Mutation:** Established the "Single active Release Specification" rule and consistent visual hierarchy for spec headings.

## [2026-02-14] v0.73 Meta-Process Evolution
- **Universal Framework Refactor:** Decoupled the `agentic_devops` framework from the specific project context. All instructions, meta-features, and tools now use project-agnostic terminology.
- **Synchronized Release Protocol:** Established a dual-domain audit requirement for GitHub pushes, ensuring application stability and DevOps toolchain health are validated in parallel.
- **Zero-Queue Mandate:** Codified the requirement that ALL features must be in the [Complete] state before a push or release.
- **Stable Application Mandate:** Formalized the ability to release process improvements (DevOps) independently of application changes.
- **Recursive Governance:** Applied the Spec-Driven Workflow to the `agentic_devops` framework itself.
- **Domain Separation:** Established `agentic_devops/features/` for meta-specifications.
- **Bootstrap Specs:** Authored initial meta-features for the agentic toolchain.
- **Granular Instruction Tracking:** Created dedicated feature specifications for `agent_architect_instructions.md` and `agent_builder_instructions.md`.
- **Agentic Orchestration:** Empowered the Builder agent to orchestrate specialized sub-agents and internal personas.
- **Test Isolation Mandate:** Enforced strict separation of test execution between the Application and Agentic (Python/Tools) domains.

## [2026-02-15] v0.74 Process Purity & Governance
- **Process Purity Mandate:** Codified strict separation between Process History (DevOps) and Firmware History (Features). `PROCESS_HISTORY.md` is now restricted to meta-process changes only.
- **Release Status Mandate:** Enforced explicit `[Complete]` tagging for Release Specifications (`RELEASE_vX.Y.md`) as a required pre-condition for closing a milestone.
- **Milestone Consolidation:** Reinforced the "Single Active Release Specification" rule by requiring the consolidation of cumulative feature milestones into a single Living Spec file before release.
- **Multi-Target Verification:** Mandated architectural parity checks across all build targets for structural changes.
- **Zero-Queue Clarification:** Explicitly stated in `ARCHITECT_INSTRUCTIONS.md` that the release status tag is `[Complete]`, not `[DONE]`.
