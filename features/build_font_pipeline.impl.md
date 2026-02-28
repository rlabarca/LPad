# Implementation Notes: Build Font Pipeline

### Source Mapping

| File | Role |
|---|---|
| `scripts/generate_theme_fonts.sh` | Font generation script |
| `src/themes/default/fonts/` | Generated `.h` output (committed artifacts) |
| `tests/build_font_pipeline/test_font_pipeline.py` | Python unit tests |

### fontconvert stdout Redirect

The script redirects `fontconvert` stdout directly to `.h` files (`> "$TARGET_DIR/Font_*.h"`). Any fake binary used in tests must write its marker output to **stderr** (not stdout) or it will be captured into the output file instead of the subprocess pipe.

### [AUTONOMOUS] No set -e on Individual Calls

Individual `fontconvert` failures produce empty `.h` files rather than aborting the entire pipeline. The spec notes this is intentional. This allows partial runs when only some fonts are available.

### [AUTONOMOUS] Hardcoded Source Directory

The source font directory (`/Users/richardlabarca/Desktop/theme_default`) is hardcoded in the script. This is a developer-local offline pipeline; the generated `.h` files are committed so CI does not need to run it. A more portable solution (e.g., accepting source dir as second argument) was not implemented as it is not required by the spec.

### Test Strategy

Because `fontconvert` requires building from source (Adafruit-GFX-Library) and has a system dependency on libfreetype, the "All 5 font headers generated" test passes by detecting committed artifacts rather than running the pipeline. This is expected and documented in the test file.
