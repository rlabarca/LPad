# Implementation Notes: Build SVG Pipeline

### Source Mapping

| File | Role |
|---|---|
| `scripts/process_svgs.py` | SVG-to-C++ converter |
| `assets/LPadLogo.svg` | Production SVG asset |
| `src/generated/vector_assets.h` | Generated header (committed) |
| `src/generated/vector_assets.cpp` | Generated source (committed) |
| `tests/build_svg_pipeline/test_svg_pipeline.py` | Python unit tests |

### [AUTONOMOUS] --assets-dir / --output-dir Arguments Added

The original script used hardcoded paths (`assets/`, `src/generated/`). During test implementation, it became clear that isolated temp-dir testing requires CLI arguments to override these paths. `argparse` was added with `--assets-dir` and `--output-dir` options; defaults preserve the original hardcoded behavior when arguments are omitted.

### Triangle-Only Path Assumption

The spec states each `<path>` represents exactly one triangle (first 3 vertices). The parser extracts all vertices from `M`/`L` commands and uses only `norm_vertices[0..2]`. Paths with fewer than 3 vertices are silently skipped via the `if len(vertices) < 3: continue` guard.

### Namespace Handling in xml.etree

SVG files with explicit namespace declarations (`xmlns="http://www.w3.org/2000-svg"`) require namespace-prefixed XPath queries. The parser tries three XPath forms (`'.//path'`, `'.//svg:path'`, `'path'`) to handle both namespaced and non-namespaced SVGs. Duplicate matches are not an issue because `findall` with a specific tag returns elements only once.

### RGB565 Packing

Formula: `(r5 << 11) | (g6 << 5) | b5` where `r5 = r >> 3`, `g6 = g >> 2`, `b5 = b >> 3`. Stored as `uint16_t` in the generated `.cpp` as a hex literal (`0xXXXX`).
