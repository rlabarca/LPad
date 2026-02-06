# Agentic Protocols for Rich (CTO/Robotics Profile)

## Executive Summary
Your mandate is to implement `features/*.md` into code and **commit to git**. The commit is the ONLY way to mark work as done. If you don't commit, it didn't happen.

---

## The Workflow Contract

**Step 0: Prime Directive**
Consult `docs/ARCHITECTURE.md` (Rules) and `docs/IMPLEMENTATION_LOG.md` (History) before writing a single line of code.

**Step 1: Recursive Dependency Check**
Check `git log` for prerequisites. If a prerequisite is `[TODO]` (due to recent changes), you **MUST** verify/re-commit it *before* the target feature. Use `git commit --allow-empty` if no code changes are needed.

**Step 2: Implement & Test**
Write code and unit tests. Run `./scripts/test_local.sh`. Pass all tests.

**Step 3: Lab Notebook**
If you made a key design decision or solved a non-obvious bug, add a concise entry to `docs/IMPLEMENTATION_LOG.md`.

**Step 4: THE COMMIT PROTOCOL (Mandatory Final Action)**
The `cdd.sh` monitor depends on EXACT string matches in commit messages.

1. `git add .`
2. **Choose the Correct Tag (Rule):**
   - **IF** the feature has a `## Hardware (HIL) Test` section OR is a 'Demo'/'App' category:
     **Tag:** `[Ready for HIL Test features/FILENAME.md]`
   - **ELSE** (pure software/logic/contract):
     **Tag:** `[Complete features/FILENAME.md]`

   *This rule applies to BOTH new implementations AND re-validation of existing features.*

3. **Execute Commit:**
   ```shell
   git commit -m "feat(<scope>): <description> <CORRECT_TAG>"
   ```
   *(Use `--allow-empty` if no code changed but status needs reset/verification).*

---

## General Directives

0.  **Efficiency:** If a task is ambiguous or token-heavy, STOP and ask for clarification.
1.  **Feature-First:** Truth lives in `features/*.md`. Follow Gherkin behaviors strictly.
2.  **HAL Barrier:** NO hardware code in `src/`. All GPIO/Display/Network must use domain-specific `hal/*.h` headers.
3.  **Visualization:** To view the current architecture, run `./scripts/serve_graph.py` and open `http://localhost:8000`.
4.  **No Chitchat:** After the commit, your turn is done. Do not explain the commit.

## Hardware Protocol
Consult `hw-examples/` for vendor sequences. Port logic directly to the HAL implementation as specified in the feature file.

## Status Reset Logic
Any edit to `features/X.md` resets its status to `[TODO]`. You MUST re-verify it and create a new status-marking commit (following the Tag Selection Rule) to clear it. Even if no code changed, if it has a HIL section, you MUST use `[Ready for HIL Test]`.
