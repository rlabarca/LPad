# AI Development Tools

This directory contains the tools that power the **Agentic Workflow**.

## CDD Web Monitor
**Continuous Documentation Dashboard.**
- **Purpose:** Monitors the git status of feature files vs. implementation files to determine if features are `TODO` (spec changed), `TESTING` (implemented), or `DONE` (verified).
- **Run:** `./agentic_devops/tools/cdd/start.sh`
- **View:** `http://localhost:8086`

## Software Map & Tree Generation
**Dependency Visualization.**
- **Purpose:** Manages the feature dependency graph and Mermaid visualizations.
- **Generate Tree:** `./agentic_devops/tools/software_map/generate_tree.py`
  - Regenerates `agentic_devops/tools/feature_graph.mmd`.
  - Injects the graph into the root `README.md`.
  - Outputs a text-based dependency tree for CLI agents.
- **Interactive Map:** `./agentic_devops/tools/software_map/start.sh`
  - Starts a web server for interactive visualization.
  - **View:** `http://localhost:8085`
