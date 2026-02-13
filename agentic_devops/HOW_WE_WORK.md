# How We Work: The LPad Agentic Workflow

## 1. Core Philosophy: "Code is Disposable"
The single source of truth for this project is not the `src/` or `hal/` code, but the **Specifications** and **Architectural Policies** stored in the root `features/` directory. 
*   If the code is lost, it must be reproducible from the specs.
*   We never fix bugs in code first; we fix the specification that allowed the bug.

## 2. Roles and Responsibilities

### The Architect (Gemini)
*   **Focus:** "The What and The Why".
*   **Ownership:** `ARCHITECT_INSTRUCTIONS.md`, `features/arch_*.md` (Policies), `features/*.md` (Requirements).
*   **Key Duty:** Designing rigorous, unambiguous specifications and enforcing architectural invariants.

### The Builder (Claude)
*   **Focus:** "The How".
*   **Ownership:** `BUILDER_INSTRUCTIONS.md`, `src/`, `hal/`, `test/`.
*   **Key Duty:** Translating specifications into high-quality, verified C++ code and documenting implementation discoveries.

### The Human Executive (Rich)
*   **Focus:** "The Intent and The Review".
*   **Duty:** Providing high-level goals, performing Hardware-in-the-Loop (HIL) verification, and managing the Agentic Evolution.

## 3. The Lifecycle of a Feature
1.  **Design:** Architect creates/refines a feature file in the root `features/` directory.
2.  **Implementation:** Builder reads the feature and implementation notes, writes code/tests, and verifies locally.
3.  **HIL Test:** User performs visual/physical verification on hardware.
4.  **Completion:** Builder commits with a status-marking tag (`[Complete]` or `[Ready for HIL Test]`).
5.  **Synchronization:** Architect updates documentation and generates the Software Map.

## 4. Knowledge Colocation
We do not use a global implementation log. Tribal knowledge, hardware "gotchas," and lessons learned are stored directly in the `## Implementation Notes` section at the bottom of each feature file.

## 5. The Release Protocol
Releases (e.g., "v0.70") are synchronization points where the entire project state—Specs, Architecture, Code, and Process—is validated and pushed to the remote repository.

1.  **Integrity Validation:** The dependency tree is refreshed, and any unlinked or "orphaned" features are staged for removal (with Human approval).
2.  **Spec-Code Audit:** The Human Executive directs the Builder to run a comprehensive integrity audit to remediate any architectural drift.
3.  **HIL Verification:** All Hardware-in-the-Loop tests defined in the release specification must be verified on physical hardware.
4.  **Documentation Sync:** `PROCESS_HISTORY.md` and the `README.md` evolution tables are synchronized to reflect the current state of the Agentic Workflow.
5.  **Git Delivery:** A formal release commit is created, and the project is pushed to GitHub.
