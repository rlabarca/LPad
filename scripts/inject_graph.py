import os
import re

def update_readme_graph(readme_path, graph_path):
    # Read the new graph content
    try:
        with open(graph_path, 'r') as f:
            graph_content = f.read().strip()
    except FileNotFoundError:
        print(f"Error: Graph file '{graph_path}' not found.")
        return

    # Read the README
    try:
        with open(readme_path, 'r') as f:
            readme_content = f.read()
    except FileNotFoundError:
        print(f"Error: README file '{readme_path}' not found.")
        return

    # Define markers
    start_marker = "<!-- MERMAID_START -->"
    end_marker = "<!-- MERMAID_END -->"

    # Create the new block
    new_block = f"{start_marker}\n```mermaid\n{graph_content}\n```\n{end_marker}"

    # Regex to find existing block between markers
    pattern = re.compile(f"{re.escape(start_marker)}.*?{re.escape(end_marker)}", re.DOTALL)

    if pattern.search(readme_content):
        # Replace existing block
        new_readme_content = pattern.sub(new_block, readme_content)
        print("Updated existing mermaid block in README.md")
    else:
        # Append if not found (fallback)
        print("Markers not found. Appending to end of file.")
        new_readme_content = readme_content + "\n" + new_block

    # Write back
    with open(readme_path, 'w') as f:
        f.write(new_readme_content)

if __name__ == "__main__":
    update_readme_graph("README.md", "feature_graph.mmd")

