# Build Scripts

## Vector Asset Pipeline

### `process_svgs.py`

Converts SVG files into optimized C++ vector data structures.

**Usage:**
```bash
python3 scripts/process_svgs.py
```

**Input:** `assets/*.svg` files containing triangulated paths

**Output:**
- `src/generated/vector_assets.h` - Header with VectorShape definitions
- `src/generated/vector_assets.cpp` - Implementation with triangle mesh data

**Requirements:**
- Python 3.6+
- SVG files with `<path>` elements using M, L, Z commands
- Each path represents a triangle (3 vertices)
- Path `fill` attribute in hex format (#RRGGBB)

**Generated Code:**
- Vertices normalized to [0.0, 1.0] range
- Colors converted to RGB565 format
- CamelCase asset names (e.g., `VectorAssets::Lpadlogo`)

**When to Run:**
- After adding/modifying any SVG files in `assets/`
- Generated files are checked into git, so this only needs to run when assets change
