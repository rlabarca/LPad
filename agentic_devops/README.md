# Agentic DevOps

This directory contains the core artifacts and instructions for the Agentic Workflow used in this project. We distinguish between two primary roles: the **Architect** (Gemini) and the **Builder** (Claude).

## Core Documentation

- **[HOW_WE_WORK.md](./HOW_WE_WORK.md)**: The high-level philosophy and operational rules for the agentic workflow.
- **[ARCHITECT_INSTRUCTIONS.md](./ARCHITECT_INSTRUCTIONS.md)**: Mandates and responsibilities for the Architect role.
- **[BUILDER_INSTRUCTIONS.md](./BUILDER_INSTRUCTIONS.md)**: Guidelines and constraints for the Builder role.
- **[PROCESS_HISTORY.md](./PROCESS_HISTORY.md)**: A log of significant changes to our development process.

## Example Workspace Setup

To facilitate this workflow, we recommend a multi-tabbed IDE setup where different agents (or roles) occupy dedicated spaces.

### 1. Architect Tab
The Architect manages the roadmap, feature specifications, and process documentation.
![Architect Tab](./Architect%20Tab.png)

### 2. Builder Tab
The Builder focuses on implementation, unit testing, and fulfilling the specifications defined by the Architect.
![Builder Tab](./Builder%20Tab.png)

### 3. Software Map Tab
A dedicated space for visualizing the system architecture and dependency graph, helping both agents understand the impact of changes.
![Software Map Tab](./Software%20Map%20Tab.png)

## Tools

The `tools/` directory contains scripts to support CDD (Continuous Deployment/Development) monitoring, software mapping, and maintenance of the feature graph.
