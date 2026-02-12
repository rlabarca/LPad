# Agentic Protocols for Rich (CTO/Robotics Profile)

## Executive Summary
Your mandate is to implement `features/*.md` into code and **commit to git**. The commit is the ONLY way to mark work as done. If you don't commit, it didn't happen.

---

## My Unbreakable Implementation & Commit Protocol

When tasked with implementing or verifying a feature from a `.md` file, you MUST follow this sequence precisely. This is your primary function.

**0. Pre-Flight Checks (Mandatory — applies to ALL code changes, not just feature implementations):**
   - **Consult the Architecture:** Before writing ANY new code, review `docs/ARCHITECTURE.md`. Key constraints include (but are not limited to):
     - §E.1: ALL drawing MUST use `RelativeDisplay` (0-100% coordinates). No raw pixel math for layout.
     - §E.2: Use off-screen canvas strategy for flicker-free rendering.
     - §E.3: No full screen clears in `loop()`. Use dirty-rect or optimized blitting.
     - §E.4: Pre-render overlays to PSRAM buffers, blit with DMA.
   - **Consult the Lab Notebook:** Before starting a complex task or bugfix, review `docs/IMPLEMENTATION_LOG.md` to see if similar problems have been solved before.
   - **Check for Dependencies:** Review the `> Prerequisite:` in the feature file and check its status with `git log`. If the prerequisite feature is marked `[TODO]` (meaning it was changed more recently than its last completion commit), you MUST stop and implement/verify the prerequisite first.

**1. Acknowledge and Plan:**
   - State which feature file you are implementing (e.g., "Implementing `features/app_config_system.md`.").
   - Briefly outline your implementation plan.

**2. Implement the Code & Document Discoveries:**
   - Write the necessary code and unit tests.
   - **Lab Notebook Rule:** If you encounter a non-obvious problem, discover a critical hardware behavior, or make a significant design decision, you MUST add a concise entry to `docs/IMPLEMENTATION_LOG.md`.

**3. Verify Locally:**
   - Run the local test suite: `./scripts/test_local.sh`.
   - You MUST confirm that all tests pass before proceeding. If they fail, debug and fix them.

**4. Commit the Work (The Final Step):**
   - This is how you signal completion. It is not optional.
   - **A. Stage all your changes:**
     ```shell
     git add .
     ```
   - **B. Determine the Correct Status Tag:**
     - Read the feature file you just implemented.
     - **IF** the file contains a section titled `## Hardware (HIL) Test` OR its `> Category:` is `Demo` or `Application`:
       - Your tag is: `[Ready for HIL Test features/THE_FILENAME.md]`
     - **ELSE** (it's a pure software component, HAL contract, etc.):
       - Your tag is: `[Complete features/THE_FILENAME.md]`
   - **C. Execute the Commit:**
     - Use the tag from the previous step to construct the final commit message.
     - The command MUST be in this exact format:
     ```shell
     git commit -m "feat(scope): your description here <THE_CORRECT_TAG_HERE>"
     ```
     *(If you are only re-validating a feature and no code has changed, add the `--allow-empty` flag to the `git commit` command.)*

**After the commit is successfully made, your task is complete. STOP and await the next instruction.**

---

## General Directives

0.  **Efficiency:** If a task is ambiguous or token-heavy, STOP and ask for clarification.
1.  **Feature-First:** Truth lives in `features/*.md`. Follow Gherkin behaviors strictly.
2.  **HAL Barrier:** NO hardware code in `src/`. All GPIO/Display/Network must use domain-specific `hal/*.h` headers.
3.  **Visualization:** To view the current architecture and navigate feature specs, start the Software Map: `./ai_dev_tools/software_map/start.sh` and open `http://localhost:8085`.
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

## Demo Architecture Pattern (CRITICAL)

**Dispatcher Pattern for Milestones:**
```
src/main.cpp                    # Dispatcher (selects demo via build flags)
  ├─ demos/demo_v05_entry.cpp   # V0.5 entry point (setup/loop)
  ├─ demos/demo_v055_entry.cpp  # V0.55 entry point (setup/loop)
  └─ demos/demo_v06_entry.cpp   # V0.6 entry point (future)

demos/v05_demo_app.cpp          # Shared core logic (Logo + 6 Graphs)
demos/v055_demo_app.cpp         # WiFi wrapper around V05DemoApp
```

**Adding a New Milestone Demo:**
1. Create `demos/demo_vXX_entry.h` and `.cpp` with `demo_setup()` and `demo_loop()`
2. Create `demos/vXX_demo_app.cpp` if new logic needed (or reuse existing)
3. Add `-DDEMO_VXX` build flag to platformio.ini environment
4. Update `main.cpp` conditional: `#elif defined(DEMO_VXX)`
5. Document in `features/demo_release_X.X.md`

**Build Flags:**
- `demo_v05_esp32s3`: `-DDEMO_V05` → uses demo_v05_entry.cpp
- `demo_v055_esp32s3`: `-DDEMO_V055` → uses demo_v055_entry.cpp
- Base `esp32s3`: Always uses latest demo (currently `-DDEMO_V055`)

**Code Sharing:**
- Maximize reuse: V0.55 wraps V0.5, adding only WiFi phase
- Never duplicate demo logic across entry points
- Shared components go in `vXX_demo_app.cpp` classes

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
