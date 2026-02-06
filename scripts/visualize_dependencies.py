import os
import re
import sys
from collections import defaultdict

def parse_features(features_dir):
    features = {}
    
    # Regex patterns
    label_pattern = re.compile(r'^>\s*Label:\s*"(.*)"')
    category_pattern = re.compile(r'^>\s*Category:\s*"(.*)"')
    prereq_pattern = re.compile(r'^>\s*Prerequisite:\s*features/(.*\.md)')
    
    for filename in os.listdir(features_dir):
        if not filename.endswith(".md"):
            continue
            
        filepath = os.path.join(features_dir, filename)
        node_id = filename.replace(".md", "").replace(".", "_")
        
        feature_data = {
            "id": node_id,
            "filename": filename,
            "label": filename, # Default
            "category": "Uncategorized", # Default
            "prerequisites": []
        }
        
        with open(filepath, 'r') as f:
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

def generate_mermaid(features):
    print("graph TD")
    print("    %% Styling")
    print("    classDef default fill:#e1f5fe,stroke:#01579b,stroke-width:1px,color:black;")
    print("    classDef release fill:#f96,stroke:#333,stroke-width:2px,color:black,font-weight:bold;")
    print("    classDef hardware fill:#e8f5e9,stroke:#2e7d32,stroke-width:1px,color:black;")
    print("    classDef ui fill:#f3e5f5,stroke:#7b1fa2,stroke-width:1px,color:black;")
    print("    classDef app fill:#fff3e0,stroke:#e65100,stroke-width:1px,color:black;")
    print("    classDef graphics fill:#e0f7fa,stroke:#006064,stroke-width:1px,color:black;")
    print("")

    # Subgraph styling is now handled by CSS in the viewer and HTML labels
    # We still keep the IDs for category-based logic if needed

    # Group by Category
    grouped_categories = defaultdict(list)
    for node_id, data in features.items():
        grouped_categories[data["category"]].append(node_id)
    
    # Render Subgraphs
    for category, node_ids in sorted(grouped_categories.items()):
        # Use HTML span with class for robust rendering and padding
        print(f"\n    subgraph {category.replace(' ', '_')} [\"<span class='subgraph-title'>{category}</span>\"]")
        print(f"        direction TB")
        for node_id in sorted(node_ids):
            data = features[node_id]
            clean_label = data["label"].replace('"', "'")
            
            # Determine class
            css_class = ""
            if "Release" in category: css_class = ":::release"
            elif "Hardware" in category: css_class = ":::hardware"
            elif "UI" in category: css_class = ":::ui"
            elif "Application" in category: css_class = ":::app"
            elif "Graphics" in category: css_class = ":::graphics"
            
            print(f'        {node_id}("**{clean_label}**<br/><small>{data["filename"]}</small>"){css_class}')
        print("    end")

    print("\n    %% Relationships")
    for node_id in sorted(features.keys()):
        data = features[node_id]
        for prereq_id in data["prerequisites"]:
            if prereq_id in features:
                print(f"    {prereq_id} --> {node_id}")
            else:
                print(f"    {prereq_id}[{prereq_id}?] -.-> {node_id}")

if __name__ == "__main__":
    features_dir = "features"
    if not os.path.exists(features_dir):
        sys.exit(1)
        
    features = parse_features(features_dir)
    generate_mermaid(features)