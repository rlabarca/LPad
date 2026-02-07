import http.server
import socketserver
import os
import json
import re
import urllib.parse
import hashlib
from collections import defaultdict

PORT = 8085
# Root is 2 levels up from this script
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../'))
MMD_FILE = os.path.join(ROOT_DIR, "ai_dev_tools/feature_graph.mmd")
FEATURES_DIR = os.path.join(ROOT_DIR, "features")
README_FILE = os.path.join(ROOT_DIR, "README.md")
SRC_DIR = os.path.join(ROOT_DIR, "src")
INCLUDE_DIR = os.path.join(ROOT_DIR, "include")
HAL_DIR = os.path.join(ROOT_DIR, "hal")

def generate_class_graph_content():
    # Regex to find includes: #include "path/to/file.h"
    include_pattern = re.compile(r'^\s*#include\s*[<"]([^>"]+)[>"]')
    # Regex for inheritance: class Name : public Base
    class_pattern = re.compile(r'\bclass\s+(\w+)\s*(?::\s*(?:public|private|protected)\s+(\w+))?\s*\{', re.DOTALL)
    
    # Map file_path -> { classes: [], includes: [], filename: "", directory: "" }
    file_map = {}
    
    dirs_to_scan = [SRC_DIR, INCLUDE_DIR, HAL_DIR]
    
    for d in dirs_to_scan:
        if not os.path.exists(d): continue
        for root, dirs, files in os.walk(d):
            for file in files:
                if file.endswith(('.h', '.hpp', '.cpp')):
                    filepath = os.path.join(root, file)
                    rel_dir = os.path.relpath(root, ROOT_DIR)
                    
                    file_info = {
                        "classes": [], # List of (ClassName, ParentNameOrNone)
                        "includes": [],
                        "filename": file,
                        "directory": rel_dir
                    }
                    
                    try:
                        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                            content = f.read()
                            
                            # Remove comments (simple C-style)
                            content = re.sub(r'//.*', '', content)
                            content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
                            
                            # Find includes
                            for line in content.split('\n'):
                                inc_match = include_pattern.match(line)
                                if inc_match:
                                    included_file = inc_match.group(1)
                                    if not included_file.startswith("Arduino") and "." in included_file:
                                        file_info["includes"].append(os.path.basename(included_file))

                            # Find class definitions
                            matches = class_pattern.finditer(content)
                            for m in matches:
                                class_name = m.group(1)
                                parent_name = m.group(2)
                                file_info["classes"].append((class_name, parent_name))
                                
                        file_map[file] = file_info
                    except Exception as e:
                        print(f"Error processing {filepath}: {e}")

    # Build Class Graph
    mermaid_lines = ["classDiagram"]
    mermaid_lines.append("    direction TB")
    mermaid_lines.append("    %% Styling")
    mermaid_lines.append("    classDef src fill:#e1f5fe,stroke:#01579b,stroke-width:1px;")
    mermaid_lines.append("    classDef hal fill:#e8f5e9,stroke:#2e7d32,stroke-width:1px;")
    mermaid_lines.append("    classDef ui fill:#f3e5f5,stroke:#7b1fa2,stroke-width:1px;")
    mermaid_lines.append("    classDef data fill:#fff3e0,stroke:#e65100,stroke-width:1px;")
    mermaid_lines.append("    classDef default fill:#f5f5f5,stroke:#333,stroke-width:1px;")
    
    # Add classes
    for filename, info in file_map.items():
        dir_name = info["directory"]
        style_class = "default"
        if "src/generated" in dir_name: style_class = "data"
        elif "src/data" in dir_name: style_class = "data"
        elif "themes" in dir_name: style_class = "ui"
        elif "src" in dir_name:
             if "ui_" in filename: style_class = "ui"
             else: style_class = "src"
        elif "hal" in dir_name: style_class = "hal"

        for cls_tuple in info["classes"]:
            cls, parent = cls_tuple
            mermaid_lines.append(f"    class {cls}")
            mermaid_lines.append(f"    class {cls}:::{style_class}")
            if parent:
                 mermaid_lines.append(f"    {cls} --|> {parent}")
            
    # Add relationships
    seen_edges = set()
    
    for filename, info in file_map.items():
        for cls_tuple in info["classes"]:
            cls, parent = cls_tuple
            
            for inc in info["includes"]:
                # 'inc' is the filename included, e.g., "display.h"
                if inc in file_map:
                    target_info = file_map[inc]
                    for target_cls_tuple in target_info["classes"]:
                        target_cls, _ = target_cls_tuple
                        if cls != target_cls and target_cls != parent:
                            # Dependency relationship
                            edge = f"    {cls} ..> {target_cls}"
                            if edge not in seen_edges:
                                mermaid_lines.append(edge)
                                seen_edges.add(edge)

    return "\n".join(mermaid_lines)

def parse_features():
    features = {}
    label_pattern = re.compile(r'^>\s*Label:\s*"(.*)"')
    category_pattern = re.compile(r'^>\s*Category:\s*"(.*)"')
    prereq_pattern = re.compile(r'^>\s*Prerequisite:\s*features/(.*\.md)')
    
    if not os.path.exists(FEATURES_DIR):
        return features

    for filename in os.listdir(FEATURES_DIR):
        if not filename.endswith(".md"):
            continue
            
        filepath = os.path.join(FEATURES_DIR, filename)
        node_id = filename.replace(".md", "").replace(".", "_")
        
        feature_data = {
            "id": node_id,
            "filename": filename,
            "label": filename,
            "category": "Uncategorized",
            "prerequisites": []
        }
        
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                label_match = label_pattern.match(line)
                if label_match:
                    feature_data["label"] = label_match.group(1)

                cat_match = category_pattern.match(line)
                if cat_match:
                    feature_data["category"] = cat_match.group(1)
                
                prereq_match = prereq_pattern.match(line)
                if prereq_match:
                    prereq_file = prereq_match.group(1)
                    prereq_id = prereq_file.replace(".md", "").replace(".", "_")
                    feature_data["prerequisites"].append(prereq_id)
        
        features[node_id] = feature_data
    return features

def generate_mermaid_content(features):
    lines = ["graph TD"]
    lines.append("    %% Styling")
    lines.append("    classDef default fill:#e1f5fe,stroke:#01579b,stroke-width:1px,color:black;")
    lines.append("    classDef release fill:#f96,stroke:#333,stroke-width:2px,color:black,font-weight:bold;")
    lines.append("    classDef hardware fill:#e8f5e9,stroke:#2e7d32,stroke-width:1px,color:black;")
    lines.append("    classDef ui fill:#f3e5f5,stroke:#7b1fa2,stroke-width:1px,color:black;")
    lines.append("    classDef app fill:#fff3e0,stroke:#e65100,stroke-width:1px,color:black;")
    lines.append("    classDef graphics fill:#e0f7fa,stroke:#006064,stroke-width:1px,color:black;")
    lines.append("")

    grouped_categories = defaultdict(list)
    for node_id, data in features.items():
        grouped_categories[data["category"]].append(node_id)
    
    for category, node_ids in sorted(grouped_categories.items()):
        # Use a spacer div to force Mermaid to allocate vertical space
        label = f"<div class='subgraph-title'>{category}</div><div style='height: 150px;'></div>"
        lines.append(f"\n    subgraph {category.replace(' ', '_')} [\"{label}\"]")
        lines.append(f"        direction TB")
        for node_id in sorted(node_ids):
            data = features[node_id]
            clean_label = data["label"].replace('"', "'")
            
            css_class = ""
            if "Release" in category: css_class = ":::release"
            elif "Hardware" in category: css_class = ":::hardware"
            elif "UI" in category: css_class = ":::ui"
            elif "Application" in category: css_class = ":::app"
            elif "Graphics" in category: css_class = ":::graphics"
            
            lines.append(f'        {node_id}("**{clean_label}**<br/><small>{data["filename"]}</small>"){css_class}')
        lines.append("    end")

    lines.append("\n    %% Relationships")
    for node_id in sorted(features.keys()):
        data = features[node_id]
        for prereq_id in data["prerequisites"]:
            if prereq_id in features:
                lines.append(f"    {prereq_id} --> {node_id}")
            else:
                lines.append(f"    {prereq_id}[{prereq_id}?] -.-> {node_id}")
    
    return "\n".join(lines)

def update_readme_and_mmd():
    features = parse_features()
    content = generate_mermaid_content(features)
    
    # Read existing MMD to check for changes
    existing_content = ""
    if os.path.exists(MMD_FILE):
        with open(MMD_FILE, 'r', encoding='utf-8') as f:
            existing_content = f.read()
            
    if content != existing_content:
        print("Graph changed. Updating MMD and README...")
        # Save MMD file
        with open(MMD_FILE, 'w', encoding='utf-8') as f:
            f.write(content)
        
        # Update README
        if os.path.exists(README_FILE):
            with open(README_FILE, 'r', encoding='utf-8') as f:
                readme_content = f.read()
                
            start_marker = "<!-- MERMAID_START -->"
            end_marker = "<!-- MERMAID_END -->"
            new_block = f"{start_marker}\n```mermaid\n{content}\n```\n{end_marker}"
            pattern = re.compile(f"{re.escape(start_marker)}.*?{re.escape(end_marker)}", re.DOTALL)
            
            if pattern.search(readme_content):
                new_readme_content = pattern.sub(new_block, readme_content)
                with open(README_FILE, 'w', encoding='utf-8') as f:
                    f.write(new_readme_content)
                print("README.md updated.")
    else:
        # Content hasn't changed, do nothing
        pass

class GraphHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # Trigger regeneration on every API request or handle via polling
        if self.path == '/api/graph':
            # Auto-regenerate before serving
            update_readme_and_mmd()
            
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            
            try:
                with open(MMD_FILE, 'r', encoding='utf-8') as f:
                    content = f.read()
                mtime = os.path.getmtime(MMD_FILE)
                response = {
                    "content": content,
                    "mtime": mtime
                }
                self.wfile.write(json.dumps(response).encode())
            except Exception as e:
                self.wfile.write(json.dumps({"error": str(e)}).encode())
        
        elif self.path == '/api/class-graph':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            
            try:
                content = generate_class_graph_content()
                # Create a hash of the content to use as version identifier
                # This ensures the frontend only re-renders if the graph actually changed
                m = hashlib.md5()
                m.update(content.encode('utf-8'))
                content_hash = m.hexdigest()
                
                response = {
                    "content": content,
                    "mtime": content_hash 
                }
                self.wfile.write(json.dumps(response).encode())
            except Exception as e:
                self.wfile.write(json.dumps({"error": str(e)}).encode())
        
        elif self.path.startswith('/api/feature/'):
            feature_name = self.path.split('/')[-1]
            feature_name = os.path.basename(feature_name)
            possible_paths = [
                os.path.join(FEATURES_DIR, feature_name),
                os.path.join(FEATURES_DIR, f"{feature_name}.md")
            ]
            
            found_path = next((p for p in possible_paths if os.path.exists(p)), None)
            
            if found_path:
                try:
                    with open(found_path, 'r', encoding='utf-8') as f:
                        file_content = f.read()
                    self.send_response(200)
                    self.send_header('Content-type', 'text/plain; charset=utf-8')
                    self.end_headers()
                    self.wfile.write(file_content.encode('utf-8'))
                except Exception as e:
                    self.send_error(500, f"Error reading file: {str(e)}")
            else:
                self.send_error(404, "Feature file not found")

        elif self.path == '/' or self.path == '/index.html':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            index_path = os.path.join(os.path.dirname(__file__), 'index.html')
            with open(index_path, 'rb') as f:
                self.copyfile(f, self.wfile)
        else:
            super().do_GET()

if __name__ == "__main__":
    print(f"Starting Software Map Server at http://localhost:{PORT}")
    os.chdir(os.path.dirname(__file__))
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), GraphHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass
        httpd.server_close()