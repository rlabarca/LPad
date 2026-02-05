import os
import re
import sys

def parse_features(features_dir):
    features = {}
    
    # Regex patterns
    label_pattern = re.compile(r'^>\s*Label:\s*"(.*)"')
    prereq_pattern = re.compile(r'^>\s*Prerequisite:\s*features/(.*\.md)')
    
    for filename in os.listdir(features_dir):
        if not filename.endswith(".md"):
            continue
            
        filepath = os.path.join(features_dir, filename)
        node_id = filename.replace(".md", "").replace(".", "_")
        
        feature_data = {
            "id": node_id,
            "filename": filename,
            "label": filename, # Default to filename
            "prerequisites": []
        }
        
        with open(filepath, 'r') as f:
            for line in f:
                label_match = label_pattern.match(line)
                if label_match:
                    feature_data["label"] = label_match.group(1)
                
                prereq_match = prereq_pattern.match(line)
                if prereq_match:
                    prereq_file = prereq_match.group(1)
                    prereq_id = prereq_file.replace(".md", "").replace(".", "_")
                    feature_data["prerequisites"].append(prereq_id)
        
        features[node_id] = feature_data
        
    return features

def generate_mermaid(features):
    print("graph TD")
    print("    %% Node Definitions")
    
    # Sort for stability
    for node_id in sorted(features.keys()):
        data = features[node_id]
        # Escape quotes in label
        clean_label = data["label"].replace('"', "'")
        
        # Style Release/Milestone nodes differently
        if "RELEASE" in node_id or "MILESTONE" in node_id:
             print(f'    {node_id}("{clean_label}<br/><small>{data["filename"]}</small>"):::release')
        else:
             print(f'    {node_id}("{clean_label}<br/><small>{data["filename"]}</small>")')

    print("\n    %% Relationships")
    for node_id in sorted(features.keys()):
        data = features[node_id]
        for prereq_id in data["prerequisites"]:
            if prereq_id in features:
                print(f"    {prereq_id} --> {node_id}")
            else:
                 # Handle external/missing prereqs gracefully
                print(f"    {prereq_id}[{prereq_id}?]) -.-> {node_id}")

    print("\n    %% Styling")
    print("    classDef release fill:#f96,stroke:#333,stroke-width:2px,color:black;")
    print("    classDef default fill:#e1f5fe,stroke:#01579b,stroke-width:1px,color:black;")

if __name__ == "__main__":
    features_dir = "features"
    if not os.path.exists(features_dir):
        print(f"Error: Directory '{features_dir}' not found.", file=sys.stderr)
        sys.exit(1)
        
    features = parse_features(features_dir)
    generate_mermaid(features)
