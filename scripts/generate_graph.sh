#!/bin/bash
set -e

# Generates the Mermaid dependency graph from feature files
python3 scripts/visualize_dependencies.py > feature_graph.mmd
echo "Graph generated at feature_graph.mmd"

# Inject into README

python3 scripts/inject_graph.py

echo "README.md updated with latest graph"



# Generate PNG

if command -v mmdc &> /dev/null

then

    mmdc -i feature_graph.mmd -o feature_graph.png -b transparent

    echo "Graph PNG generated at feature_graph.png"

else

    echo "mmdc not found, skipping PNG generation"

fi
