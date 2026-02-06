# Build & Utility Scripts

## Visualization & Development

### `serve_graph.py` & `graph_viewer.html`
Starts a local live-reloading HTTP server to view the feature dependency graph.
- **Run:** `python3 scripts/serve_graph.py`
- **View:** `http://localhost:8000`
- **Features:** Zoom, pan, and automatic refresh when `feature_graph.mmd` changes.

### `generate_graph.sh`
Orchestrates the regeneration of the Mermaid graph from `features/*.md`.
- **Run:** `./scripts/generate_graph.sh`
- **Result:** Updates `feature_graph.mmd` and injects it into `README.md`.

### `test_local.sh`
Runs the native unit tests using PlatformIO.
- **Run:** `./scripts/test_local.sh`

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
