#!/usr/bin/env python3
"""
SVG to C++ Vector Asset Converter
Parses SVG files containing triangulated paths and generates optimized C++ data structures.
"""

import xml.etree.ElementTree as ET
import re
import os
from pathlib import Path
from typing import List, Tuple, Optional


class VectorVertex:
    def __init__(self, x: float, y: float):
        self.x = x
        self.y = y

    def __repr__(self):
        return f"VectorVertex({self.x:.6f}, {self.y:.6f})"


class VectorTriangle:
    def __init__(self, v1: VectorVertex, v2: VectorVertex, v3: VectorVertex):
        self.v1 = v1
        self.v2 = v2
        self.v3 = v3


class VectorPath:
    def __init__(self, color_rgb565: int, triangles: List[VectorTriangle]):
        self.color = color_rgb565
        self.triangles = triangles


class VectorShape:
    def __init__(self, name: str, paths: List[VectorPath], width: float, height: float):
        self.name = name
        self.paths = paths
        self.original_width = width
        self.original_height = height


def hex_to_rgb565(hex_color: str) -> int:
    """Convert hex color (#RRGGBB) to RGB565 (uint16_t)."""
    hex_color = hex_color.lstrip('#')
    r = int(hex_color[0:2], 16)
    g = int(hex_color[2:4], 16)
    b = int(hex_color[4:6], 16)

    # Convert 8-bit RGB to 5-6-5 bit RGB
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F

    # Pack into 16-bit value: RRRRRGGGGGGBBBBB
    rgb565 = (r5 << 11) | (g6 << 5) | b5
    return rgb565


def parse_path_data(d: str) -> List[Tuple[float, float]]:
    """
    Parse SVG path 'd' attribute containing M, L, Z commands.
    Returns list of (x, y) coordinate tuples.
    """
    vertices = []

    # Remove commas and split by whitespace
    d = d.replace(',', ' ')
    tokens = d.split()

    i = 0
    current_x, current_y = 0.0, 0.0

    while i < len(tokens):
        cmd = tokens[i]

        if cmd == 'M':  # MoveTo
            current_x = float(tokens[i+1])
            current_y = float(tokens[i+2])
            vertices.append((current_x, current_y))
            i += 3
        elif cmd == 'L':  # LineTo
            current_x = float(tokens[i+1])
            current_y = float(tokens[i+2])
            vertices.append((current_x, current_y))
            i += 3
        elif cmd == 'Z' or cmd == 'z':  # ClosePath
            i += 1
        else:
            # Try to parse as numeric coordinate (implicit LineTo)
            try:
                current_x = float(tokens[i])
                current_y = float(tokens[i+1])
                vertices.append((current_x, current_y))
                i += 2
            except (ValueError, IndexError):
                i += 1

    return vertices


def parse_svg_file(svg_path: Path) -> Optional[VectorShape]:
    """Parse an SVG file and extract vector shape data."""
    try:
        tree = ET.parse(svg_path)
        root = tree.getroot()

        # Extract viewBox or width/height
        viewbox = root.get('viewBox')
        if viewbox:
            _, _, width, height = map(float, viewbox.split())
        else:
            width = float(root.get('width', '100'))
            height = float(root.get('height', '100'))

        # Find all path elements
        paths = []
        namespace = {'svg': 'http://www.w3.org/2000/svg'}

        for path_elem in root.findall('.//path', namespace) + root.findall('.//svg:path', namespace) + root.findall('path'):
            fill = path_elem.get('fill')
            d = path_elem.get('d')

            if not fill or not d or fill.lower() == 'none':
                continue

            # Parse path data
            vertices = parse_path_data(d)

            if len(vertices) < 3:
                continue

            # Convert to triangle (assuming each path is already a triangle)
            if len(vertices) >= 3:
                # Normalize coordinates to [0, 1]
                norm_vertices = [
                    VectorVertex(x / width, y / height) for x, y in vertices
                ]

                # Create triangle from first 3 vertices
                triangle = VectorTriangle(
                    norm_vertices[0],
                    norm_vertices[1],
                    norm_vertices[2]
                )

                # Convert color
                rgb565 = hex_to_rgb565(fill)

                paths.append(VectorPath(rgb565, [triangle]))

        if not paths:
            return None

        # Generate name from filename (remove extension, convert to CamelCase)
        name = svg_path.stem
        # Convert snake_case or kebab-case to CamelCase
        name_parts = re.split(r'[-_]', name)
        name = ''.join(part.capitalize() for part in name_parts)

        return VectorShape(name, paths, width, height)

    except Exception as e:
        print(f"Error parsing {svg_path}: {e}")
        return None


def generate_cpp_header(shapes: List[VectorShape]) -> str:
    """Generate the vector_assets.h header file."""
    lines = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "#include <stddef.h>",
        "",
        "// Auto-generated by scripts/process_svgs.py",
        "// DO NOT EDIT MANUALLY",
        "",
        "struct VectorVertex {",
        "    float x; // 0.0 to 1.0",
        "    float y; // 0.0 to 1.0",
        "};",
        "",
        "struct VectorTriangle {",
        "    VectorVertex v1;",
        "    VectorVertex v2;",
        "    VectorVertex v3;",
        "};",
        "",
        "struct VectorPath {",
        "    uint16_t color;      // RGB565",
        "    size_t num_tris;",
        "    const VectorTriangle* tris;",
        "};",
        "",
        "struct VectorShape {",
        "    size_t num_paths;",
        "    const VectorPath* paths;",
        "    float original_width;",
        "    float original_height;",
        "};",
        "",
        "namespace VectorAssets {",
        ""
    ]

    for shape in shapes:
        lines.append(f"    extern const VectorShape {shape.name};")

    lines.extend([
        "",
        "} // namespace VectorAssets",
        ""
    ])

    return '\n'.join(lines)


def generate_cpp_source(shapes: List[VectorShape]) -> str:
    """Generate the vector_assets.cpp source file."""
    lines = [
        "#include \"vector_assets.h\"",
        "",
        "// Auto-generated by scripts/process_svgs.py",
        "// DO NOT EDIT MANUALLY",
        "",
        "namespace VectorAssets {",
        ""
    ]

    for shape in shapes:
        # Generate triangle data for each path
        for path_idx, path in enumerate(shape.paths):
            tri_array_name = f"{shape.name}_path{path_idx}_tris"
            lines.append(f"    static const VectorTriangle {tri_array_name}[] = {{")

            for tri in path.triangles:
                lines.append(f"        {{ {{{tri.v1.x:.6f}f, {tri.v1.y:.6f}f}}, "
                           f"{{{tri.v2.x:.6f}f, {tri.v2.y:.6f}f}}, "
                           f"{{{tri.v3.x:.6f}f, {tri.v3.y:.6f}f}} }},")

            lines.append("    };")
            lines.append("")

        # Generate path array
        paths_array_name = f"{shape.name}_paths"
        lines.append(f"    static const VectorPath {paths_array_name}[] = {{")

        for path_idx, path in enumerate(shape.paths):
            tri_array_name = f"{shape.name}_path{path_idx}_tris"
            lines.append(f"        {{ 0x{path.color:04X}, "
                       f"{len(path.triangles)}, {tri_array_name} }},")

        lines.append("    };")
        lines.append("")

        # Generate shape definition
        lines.append(f"    const VectorShape {shape.name} = {{")
        lines.append(f"        {len(shape.paths)},")
        lines.append(f"        {paths_array_name},")
        lines.append(f"        {shape.original_width:.1f}f,")
        lines.append(f"        {shape.original_height:.1f}f")
        lines.append("    };")
        lines.append("")

    lines.extend([
        "} // namespace VectorAssets",
        ""
    ])

    return '\n'.join(lines)


def main():
    """Main entry point."""
    # Get project root (script is in scripts/ directory)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    assets_dir = project_root / "assets"
    output_dir = project_root / "src" / "generated"

    # Create output directory if needed
    output_dir.mkdir(parents=True, exist_ok=True)

    # Find all SVG files
    svg_files = list(assets_dir.glob("*.svg"))

    if not svg_files:
        print(f"No SVG files found in {assets_dir}")
        return 1

    print(f"Processing {len(svg_files)} SVG file(s)...")

    # Parse all SVGs
    shapes = []
    for svg_path in sorted(svg_files):
        print(f"  - {svg_path.name}")
        shape = parse_svg_file(svg_path)
        if shape:
            shapes.append(shape)
            print(f"    -> {shape.name} ({len(shape.paths)} paths)")

    if not shapes:
        print("No valid shapes found!")
        return 1

    # Generate C++ files
    header_path = output_dir / "vector_assets.h"
    source_path = output_dir / "vector_assets.cpp"

    print(f"\nGenerating {header_path}...")
    with open(header_path, 'w') as f:
        f.write(generate_cpp_header(shapes))

    print(f"Generating {source_path}...")
    with open(source_path, 'w') as f:
        f.write(generate_cpp_source(shapes))

    print("\nDone!")
    return 0


if __name__ == "__main__":
    exit(main())
