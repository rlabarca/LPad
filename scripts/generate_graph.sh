#!/bin/bash
# Generates the Mermaid dependency graph from feature files
python3 scripts/visualize_dependencies.py > feature_graph.mmd
echo "Graph generated at feature_graph.mmd"
