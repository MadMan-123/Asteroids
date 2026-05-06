#!/usr/bin/env python3
"""
Blender batch LOD generator - reads FBX, creates LODs, exports FBX.
Run with: blender --background --python batch_lods_blender.py
"""

import bpy
import os
from pathlib import Path

INPUT_DIR = "C:/Dev/DataOrientedEngineHonours/res/Models/asteroids"
OUTPUT_DIR = "C:/Dev/DataOrientedEngineHonours/res"

LOD1_RATIO = 0.25
LOD2_RATIO = 0.05

def count_tris(obj):
    if obj.type != 'MESH':
        return 0
    return len(obj.data.polygons)

def process_fbx(fbx_file, output_dir, lod1_ratio=0.25, lod2_ratio=0.05):
    try:
        print(f"\nProcessing: {fbx_file.name}")
        
        bpy.ops.object.select_all(action='SELECT')
        bpy.ops.object.delete(use_global=False)
        
        bpy.ops.import_scene.fbx(filepath=str(fbx_file))
        
        mesh_obj = bpy.context.selected_objects[0] if bpy.context.selected_objects else None
        if not mesh_obj or mesh_obj.type != 'MESH':
            print(f"  ✗ No mesh found")
            return False
        
        bpy.context.view_layer.objects.active = mesh_obj
        original_tris = count_tris(mesh_obj)
        print(f"  Original: {original_tris:,} triangles")
        
        base_name = fbx_file.stem
        
        # LOD1
        print(f"  Creating LOD1...")
        bpy.ops.object.duplicate()
        lod1_obj = bpy.context.active_object
        lod1_obj.name = f"{base_name}_LOD1"
        bpy.context.view_layer.objects.active = lod1_obj
        
        decimate = lod1_obj.modifiers.new(name="Decimate", type='DECIMATE')
        decimate.decimate_type = 'COLLAPSE'
        decimate.ratio = lod1_ratio
        bpy.ops.object.modifier_apply(modifier=decimate.name)
        
        lod1_tris = count_tris(lod1_obj)
        print(f"    Result: {lod1_tris:,} triangles")
        
        lod1_path = os.path.join(output_dir, f"{base_name}_LOD1.fbx")
        bpy.ops.object.select_all(action='DESELECT')
        lod1_obj.select_set(True)
        bpy.context.view_layer.objects.active = lod1_obj
        bpy.ops.export_scene.fbx(filepath=lod1_path, use_selection=True)
        print(f"    ✓ Exported LOD1")
        
        # LOD2
        print(f"  Creating LOD2...")
        bpy.ops.object.select_all(action='DESELECT')
        mesh_obj.select_set(True)
        bpy.context.view_layer.objects.active = mesh_obj
        bpy.ops.object.duplicate()
        lod2_obj = bpy.context.active_object
        lod2_obj.name = f"{base_name}_LOD2"
        bpy.context.view_layer.objects.active = lod2_obj
        
        decimate = lod2_obj.modifiers.new(name="Decimate", type='DECIMATE')
        decimate.decimate_type = 'COLLAPSE'
        decimate.ratio = lod2_ratio
        bpy.ops.object.modifier_apply(modifier=decimate.name)
        
        lod2_tris = count_tris(lod2_obj)
        print(f"    Result: {lod2_tris:,} triangles")
        
        lod2_path = os.path.join(output_dir, f"{base_name}_LOD2.fbx")
        bpy.ops.object.select_all(action='DESELECT')
        lod2_obj.select_set(True)
        bpy.context.view_layer.objects.active = lod2_obj
        bpy.ops.export_scene.fbx(filepath=lod2_path, use_selection=True)
        print(f"    ✓ Exported LOD2")
        
        return True
        
    except Exception as e:
        print(f"  ✗ Error: {str(e)}")
        return False

def main():
    fbx_files = list(Path(INPUT_DIR).glob("*.fbx"))
    
    if not fbx_files:
        print(f"No FBX files found")
        return
    
    print(f"\nFound {len(fbx_files)} FBX files")
    
    success = 0
    for i, fbx_file in enumerate(sorted(fbx_files), 1):
        print(f"\n[{i}/{len(fbx_files)}]")
        if process_fbx(fbx_file, OUTPUT_DIR, LOD1_RATIO, LOD2_RATIO):
            success += 1
    
    print(f"\n\nComplete! {success}/{len(fbx_files)} successful\n")

if __name__ == "__main__":
    main()
