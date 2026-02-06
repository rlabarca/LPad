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

## PlatformIO Build Configuration (Critical)

### Demo Environment Naming Convention
All milestone demos use the pattern: `demo_<version>_<board>`

**Examples:**
- `demo_v05_esp32s3` - v0.5 demo on ESP32-S3 AMOLED board
- `demo_v05_tdisplay` - v0.5 demo on T-Display S3 Plus
- `demo_v055_esp32s3` - v0.55 demo on ESP32-S3 AMOLED board
- `demo_v055_tdisplay` - v0.55 demo on T-Display S3 Plus

**To build a specific milestone demo:**
```bash
pio run -e demo_v05_esp32s3    # Build v0.5 for ESP32-S3 AMOLED
pio run -e demo_v055_tdisplay  # Build v0.55 for T-Display S3 Plus
```

**Demo Version Differences:**
- **v0.5**: Logo → Graph Modes → Loop (no network/wifi)
- **v0.55**: WiFi Connectivity → Logo → Graph Modes → Loop (includes network smoke test)

### Main Hardware Environments
The base hardware environments (`esp32s3`, `tdisplay_s3_plus`) **ALWAYS** reference the LATEST demo in `main.cpp`.

**ALWAYS ensure `main.cpp` links to the latest demo for ALL hardware target environments.**

When creating new demo apps (e.g., `V055DemoApp`, `V06DemoApp`):
1. Update `main.cpp` to use the new latest demo
2. Update ALL base hardware environment `build_src_filter` sections (`esp32s3`, `tdisplay_s3_plus`)
3. Create new `demo_<version>_<board>` environments for the new milestone

**Required pattern for base hardware environments:**
```ini
build_src_filter =
    +<*>
    +<main.cpp>                      # Include main entry point
    +<../hal/*.cpp>                  # Include HAL implementations
    -<../hal/display_stub.cpp>       # Exclude stubs for hardware builds
    -<../hal/timer_stub.cpp>
    -<../hal/network_stub.cpp>
    +<../demos/v0XX_demo_app.cpp>    # Include latest demo coordinator
    +<../demos/v0X_demo_app.cpp>     # Include demo helper classes
```

**DO NOT:**
- Use `-<main.cpp>` in base hardware environments (this excludes the main entry point)
- Use `+<../demos/demo_screen.cpp>` in base hardware environments (old monolithic demo pattern)
- Leave old demo files included when migrating to new demo architecture

**Check these environments:** `esp32s3`, `tdisplay_s3_plus`, and any other hardware targets.

## Hardware Upload Protocol
**DO NOT automatically upload to devices.** The user controls which device is connected via USB.

**Your role:**
1. Build the firmware: `pio run -e <env>`
2. **STOP and inform the user** that the build is ready
3. Tell the user: "Ready to upload. Run: `pio run -e <env> -t upload`"
4. Let the user execute the upload command when they're ready

**Only upload automatically if:**
- The user explicitly asks you to upload
- The user provides a specific environment to upload to

## Status Reset Logic
Any edit to `features/X.md` resets its status to `[TODO]`. You MUST re-verify it and create a new status-marking commit (following the Tag Selection Rule) to clear it. Even if no code changed, if it has a HIL section, you MUST use `[Ready for HIL Test]`.
