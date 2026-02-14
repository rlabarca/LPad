# How We Work: The LPad Agentic Workflow

## 1. Core Philosophy: "Code is Disposable"
The single source of truth for this project is not the code, but the **Specifications** and **Architectural Policies**. 
*   **Application Domain:** `features/` defines the LPad product.
*   **Agentic Domain:** `agentic_devops/features/` defines the workflow and tools.
*   If the code is lost, it must be reproducible from the specs.
*   We never fix bugs in code first; we fix the specification that allowed the bug.

## 2. Roles and Responsibilities

### The Architect (Gemini)
*   **Focus:** "The What and The Why".
*   **Ownership:** `ARCHITECT_INSTRUCTIONS.md`, `features/arch_*.md`, `features/*.md`, and `agentic_devops/features/`.
*   **Key Duty:** Designing rigorous, unambiguous specifications and enforcing architectural invariants across both domains.

### The Builder (Claude)
*   **Focus:** "The How".
*   **Ownership:** `BUILDER_INSTRUCTIONS.md`, `src/`, `hal/`, `test/`, and `agentic_devops/tools/`.
*   **Key Duty:** Translating specifications into high-quality, verified code and documenting implementation discoveries.

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

### 5.1 Milestone Mutation (The "Single Release File" Rule)
We do not maintain a history of release files in the `features/` directory.
1. There is exactly ONE active Release Specification file (e.g., `RELEASE_v0.72_name.md`).
2. When moving to a new release, the Architect **renames** the existing release file to the new version and updates the objectives.
3. The previous release's HIL tests are preserved as **Regression Tests** in the new file.
4. Historical release data is tracked via `agentic_devops/PROCESS_HISTORY.md` and the root `README.md`.
