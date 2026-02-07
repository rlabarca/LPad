# AI Development Tools

This directory contains the tools that power the **Agentic Workflow**.

## `cdd.sh`
**Continuous Documentation Dashboard.**
- **Purpose:** Monitors the git status of feature files vs. implementation files to determine if features are `TODO` (spec changed), `TESTING` (implemented), or `DONE` (verified).
- **Run:** `./ai_dev_tools/cdd.sh`

## `software_map/`
**Interactive Software Map.**
- **Purpose:** Visualizes the dependency graph of feature files and provides a UI for navigating specifications.
- **Run:** `./ai_dev_tools/software_map/start.sh`
- **View:** `http://localhost:8085`
- **Auto-Sync:** Automatically regenerates the Mermaid graph in the root `README.md` when accessed.
