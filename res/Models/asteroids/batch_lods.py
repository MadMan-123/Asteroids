#!/usr/bin/env python3
"""
Batch LOD generator using trimesh with FBX support via pyassimp.
Processes all FBX files in a folder and creates LOD versions.
"""

import trimesh
import os
from pathlib import Path

# Try to import assimp for FBX support
HAS_ASSIMP = False
try:
    from pyassimp import load as assimp_load, release as assimp_release
    HAS_ASSIMP = True
    print("✓ pyassimp imported successfully")
except ImportError as e:
    print(f"WARNING: Could not import pyassimp: {e}")

# ============================================================================
# CONFIG
# ============================================================================

INPUT_DIR = "C:/Dev/DataOrientedEngineHonours/res/Models/asteroids"
OUTPUT_DIR = "C:/Dev/DataOrientedEngineHonours/res"

LOD1_RATIO = 0.25  # 25%
LOD2_RATIO = 0.05  # 5%

# ============================================================================

def load_fbx_with_assimp(filepath):
    """Load FBX file using assimp."""
    scene = assimp_load(str(filepath))
    meshes = []
    
    for mesh in scene.meshes:
        verts = mesh.vertices
        faces = mesh.faces
        m = trimesh.Trimesh(vertices=verts, faces=faces, process=False)
        meshes.append(m)
    
    assimp_release(scene)
    
    if len(meshes) == 1:
        return meshes[0]
    else:
        return trimesh.util.concatenate(meshes)


def process_model(filepath, output_dir, lod1_ratio=0.25, lod2_ratio=0.05):
    """Process a single model and create LOD versions."""
    
    try:
        print(f"\nProcessing: {filepath.name}")
        
        # Load mesh
        if str(filepath).lower().endswith('.fbx'):
            if not HAS_ASSIMP:
                print(f"  ✗ Error: pyassimp required for FBX. Install with: pip install pyassimp")
                return False
            mesh = load_fbx_with_assimp(filepath)
        else:
            mesh = trimesh.load(str(filepath), process=False)
            if isinstance(mesh, trimesh.Scene):
                mesh = trimesh.util.concatenate([g for g in mesh.geometry.values()])
        
        base_name = filepath.stem
        
        original_faces = len(mesh.faces)
        print(f"  Original: {original_faces:,} faces")
        
        # ====================================================================
        # LOD1
        # ====================================================================
        print(f"  Creating LOD1 ({lod1_ratio*100:.0f}%)...")
        target_lod1 = max(int(original_faces * lod1_ratio), 3)
        lod1_mesh = mesh.simplify_quadratic_mesh(target_count=target_lod1)
        lod1_faces = len(lod1_mesh.faces)
        
        lod1_path = os.path.join(output_dir, f"{base_name}_LOD1.fbx")
        lod1_mesh.export(lod1_path)
        print(f"    ✓ {lod1_faces:,} faces ({lod1_faces/original_faces*100:.1f}%) → {base_name}_LOD1.fbx")
        
        # ====================================================================
        # LOD2
        # ====================================================================
        print(f"  Creating LOD2 ({lod2_ratio*100:.0f}%)...")
        target_lod2 = max(int(original_faces * lod2_ratio), 3)
        lod2_mesh = mesh.simplify_quadratic_mesh(target_count=target_lod2)
        lod2_faces = len(lod2_mesh.faces)
        
        lod2_path = os.path.join(output_dir, f"{base_name}_LOD2.fbx")
        lod2_mesh.export(lod2_path)
        print(f"    ✓ {lod2_faces:,} faces ({lod2_faces/original_faces*100:.1f}%) → {base_name}_LOD2.fbx")
        
        return True
        
    except Exception as e:
        print(f"  ✗ Error: {str(e)}")
        import traceback
        traceback.print_exc()
        return False


def main():
    """Process all models in input directory."""
    
    if not os.path.exists(INPUT_DIR):
        print(f"ERROR: Input directory not found: {INPUT_DIR}")
        return
    
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # Find all model files
    extensions = ('*.glb', '*.fbx', '*.obj', '*.ply')
    files = []
    for ext in extensions:
        files.extend(Path(INPUT_DIR).glob(ext))
    
    # Also check case-insensitive
    for ext in ('*.FBX', '*.GLB', '*.OBJ', '*.PLY'):
        files.extend(Path(INPUT_DIR).glob(ext))
    
    # Remove duplicates
    files = list(set(files))
    
    if not files:
        print(f"ERROR: No model files found in {INPUT_DIR}")
        print(f"Looking in: {INPUT_DIR}")
        print(f"Files in directory: {list(Path(INPUT_DIR).iterdir())[:10]}")
        return
    
    print(f"Found {len(files)} model files")
    
    print(f"\n{'='*70}")
    print(f"Batch LOD Generator")
    print(f"{'='*70}")
    print(f"Input directory:  {INPUT_DIR}")
    print(f"Output directory: {OUTPUT_DIR}")
    print(f"Files found:      {len(files)}")
    print(f"LOD1 ratio:       {LOD1_RATIO*100:.0f}%")
    print(f"LOD2 ratio:       {LOD2_RATIO*100:.0f}%")
    print(f"{'='*70}")
    
    success = 0
    failed = 0
    
    for i, filepath in enumerate(sorted(files), 1):
        print(f"\n[{i}/{len(files)}]")
        if process_model(filepath, OUTPUT_DIR, LOD1_RATIO, LOD2_RATIO):
            success += 1
        else:
            failed += 1
    
    print(f"\n{'='*70}")
    print(f"Complete!")
    print(f"{'='*70}")
    print(f"Successful: {success}")
    print(f"Failed:     {failed}")
    print(f"Total:      {len(files)}")
    print(f"{'='*70}\n")


if __name__ == "__main__":
    main()