#!/usr/bin/env python3
import os
import re
import sys

"""
cleanup_orphaned_features.py
Identifies and optionally moves .md files in the features/ directory 
that are not part of the dependency tree to a .trash folder.
"""

def get_referenced_features(features_dir):
    all_files = {f for f in os.listdir(features_dir) if f.endswith(".md")}
    is_prerequisite = set()
    
    for filename in all_files:
        path = os.path.join(features_dir, filename)
        with open(path, 'r') as f:
            content = f.read()
            # Find lines like: > Prerequisite: features/target.md
            matches = re.findall(r'> Prerequisite:\s*features/([a-zA-Z0-9_\-\.]+)', content)
            for m in matches:
                is_prerequisite.add(m)
                
    # Root nodes we want to keep even if nothing points to them:
    # - arch_*.md (Policies)
    # - RELEASE_*.md (Active or historical releases)
    protected_roots = {f for f in all_files if f.startswith("arch_") or f.startswith("RELEASE_")}
    
    orphans = all_files - is_prerequisite - protected_roots
    
    # Final filter: Ignore files that contain [IN_PROGRESS] tag
    true_orphans = set()
    for o in orphans:
        path = os.path.join(features_dir, o)
        with open(path, 'r') as f:
            if "[IN_PROGRESS]" not in f.read():
                true_orphans.add(o)
    
    return true_orphans

def main():
    features_dir = "features"
    trash_dir = os.path.join(features_dir, ".trash")
    
    if not os.path.exists(features_dir):
        print(f"Error: {features_dir} directory not found.")
        sys.exit(1)

    orphans = get_referenced_features(features_dir)
    
    if not orphans:
        print("No orphaned feature files found.")
        return

    print("The following feature files appear to be ORPHANED (not linked, not policy, not release):")
    for o in sorted(orphans):
        print(f"  - {o}")

    if "--fix" in sys.argv:
        if not os.path.exists(trash_dir):
            os.makedirs(trash_dir)
        for o in orphans:
            os.rename(os.path.join(features_dir, o), os.path.join(trash_dir, o))
        print(f"\nMoved {len(orphans)} files to {trash_dir}.")
        print("Review these files before manual deletion.")
    else:
        print("\nRun with '--fix' to move these to .trash for review.")

if __name__ == "__main__":
    main()
