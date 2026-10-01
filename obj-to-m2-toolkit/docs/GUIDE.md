# Conversion guide

## 1. Establish scope and target paths

Work in an asset workspace and a fresh output folder. Keep source OBJ, MTL, images and Blender files intact. Work on generated copies. Do not use a live game Data folder as staging.

Identify the exact M2 path used by the world placement. Use an MPQ listfile, a model viewer and/or the map's placement data. Inspect the reference model's vertex bounds and the placement's scale. The model's raw height is not necessarily its height in the world. Replacing a path affects every placement that uses it, not just the tree in one screenshot.

Use MPQEditor to extract the identified reference M2 to a scratch folder, without changing the archive, then measure it:

```powershell
python scripts\pipeline.py measure reference.m2
```

This reports raw vertex bounds, not placement scale or animation extent. No Blizzard reference assets need to be shipped in your deliverable.

The model path contains the filename and all archive directories, for example:

```
World\Azeroth\Elwynn\PassiveDoodads\Trees\CanopylessTree01.m2
World\Azeroth\Elwynn\PassiveDoodads\Trees\CanopylessTree0100.skin
World\CustomTrees\my-oak_basecolor.blp
```

The `00.skin` suffix is appended to the full model stem; the repeated digits above are intentional. Keep M2, matching SKIN and referenced BLP together.

## 2. Prepare and inspect the OBJ

Export triangles, UV coordinates and normals. Preserve UV seams. Apply the desired source transforms deliberately and record the up axis. The workflow supports Y-up and Z-up sources. Negative OBJ indices are supported, but input must already be triangulated.

```powershell
python scripts\pipeline.py inspect "D:\Assets\oak.obj"
```

Use the inspection to check triangle count, dimensions and the presence of UVs/normals. Blender's polygon count may count quads; two triangles per quad is the usual export result. Our current converter limits a model to 65,535 exported vertices and 65,535 triangle indices (21,845 triangles). UV seams and hard normals increase exported vertex count. These are current converter limitations, not proven universal limits of either game client.

This pipeline uses one opaque base-color texture per model. It does not import Blender PBR graphs, automatically combine multiple material atlases, or use metallic, roughness and normal maps. Bake/prepare a single atlas before conversion when necessary. Alpha-cutout leaves require a separate material-aware workflow; the texture encoder rejects nonopaque source alpha rather than silently stripping it.

## 3. Configure scale, origin and ground contact

Copy `examples/tree-template.json` to your source folder. Replace every placeholder. Fields:

| Field | Meaning |
|---|---|
| `obj`, `texture` | Source paths relative to this JSON |
| `model_path`, `texture_path` | Paths inside the MPQ, not filesystem paths |
| `up_axis` | `Y` rotates `(x,y,z)` to `(x,-z,y)`; `Z` leaves orientation unchanged |
| `origin` | Three values to subtract AFTER rotation but BEFORE scaling |
| `scale` | Positive X/Y/Z scale factors |
| `extend_foot` | Extend vertices near local ground z=0 down by this amount in model units |
| `lower_z` | Move the entire visible model AND collision down by this many model units |
| `texture_size` | Power of two, normally 2048 for our current tested baseline |
| `ue_dark_pixel_guard` | Optional asset-side workaround described in compatibility notes |
| `collision` | Explicit collision mode and its parameters |

Locate the trunk's ground center, not just the center of its canopy. Set `origin` to that point expressed in rotated Z-up coordinates. Keep the ground plane near z=0. For uniform scaling, divide the desired height by source height and use that factor on all axes. If the crown would become too wide, choose a compromise deliberately. Nonuniform scaling changes the tree's proportions; the tool transforms normals with inverse scale and renormalizes them.

Foot extension stretches only the lowest narrow band of vertices. It is not terrain conformation. The pipeline preserves source normals through this small foot edit, so inspect shading if the edit is large. Lowering moves the whole tree. World placement scales also scale the lowering amount. Never describe a two-unit asset shift as exactly two real-world meters at every placement.

A large root apron may need reshaping rather than repeated lowering. A single fixed model cannot match every slope. Prefer root undersides that extend below ground while keeping the trunk upright.

## 4. Choose collision deliberately

Automatic tree mode:

```json
"collision": {
  "mode": "tree",
  "top_fraction": 0.22,
  "sample_floor_fraction": 0.0005
}
```

The helper samples 64 radial directions at several heights. `top_fraction` chooses the upper limit as a fraction of the prepared tree's height. Keep it below substantial branch/foliage geometry. `sample_floor_fraction` lets the lowest rings use a more stable cross-section when the lowest source vertex is an isolated root tip.

The helper assumes the trunk is centered around the local origin. It approximates concave root shapes and bridges narrow gaps. It may fail on off-center, forked, disconnected or irregular trunks. Do not relax validation to force such a model through: recenter it, choose another sample floor, or supply a hand-made proxy. The automatic collision is not guaranteed to fit an arbitrary mesh.

For an authored proxy:

```json
"collision": {"mode": "prepared_obj", "path": "oak-trunk-collision.obj"}
```

That OBJ must already be a closed, triangulated proxy in output Z-up model coordinates BEFORE `lower_z`. The tool does not apply source rotation, scale or origin to this prepared proxy; it only applies the final lowering to both visible and collision meshes. Use positive face indices. Include trunk and walkable root slopes, exclude leaves. Closed topology is checked. Gameplay walkability depends on client behavior and must be tested.

## 5. Build the model and texture

```powershell
python scripts\pipeline.py build "D:\Assets\tree.json" --out builds\tree-001
```

The builder saves the transformed OBJ and collision OBJ under `prepared/`, invokes OBJtoM2 with an explicit BLP reference, writes a static sequence with bounds and creates the matching SKIN. It never modifies source files. `build-config.json`, `model-report.json` and converter logs record the choices and input hashes.

For texture-only conversion:

```powershell
python scripts\pipeline.py blp oak_basecolor.jpg oak_basecolor.blp --size 2048
```

Add `--ue-dark-pixel-guard` only for the documented UE material behavior. The encoder produces opaque BLP2 BC1/DXT1 with a complete mip chain, including padded 2x2 and 1x1 levels. It forces four-color BC1 blocks. Conversion is lossy, so inspect the decoded texture against the source.

## 6. Validate and package

`build` automatically validates and packs. To validate or pack a staging directory separately:

```powershell
python scripts\pipeline.py validate builds\tree-001\archive
python scripts\pipeline.py pack builds\tree-001\archive builds\repacked\patch-enGB-T.MPQ
python scripts\pipeline.py verify builds\repacked\patch-enGB-T.MPQ builds\tree-001\archive
```

Checks include file signatures/version, array ranges, geometry indices, bounds, unit normals, static sequence bounds, texture references, BLP mip ranges, opaque four-color encoding, collision closure, MPQ member paths and exact extracted bytes. SHA-256 values are recorded. Unsupported extra files in the archive staging directory are rejected.

For a combined patch, either put all source models in one configuration or copy complete model/SKIN/texture sets into a NEW staging folder and use `pack`. Use unique texture paths and check for case-insensitive collisions. Do not copy the old MPQ itself into staging. Never overwrite only the M2 while retaining an unrelated SKIN.

## 7. Client test and rollback

Close both clients. Back up the previous custom patch OUTSIDE all scanned Data folders. Copy the new `patch-enGB-T.MPQ` into the test client's `Data/enGB`. The toolkit intentionally does not automate installation.

Check the replacement at known placements in BOTH stock WoW 3.3.5a and the UE client: appearance from multiple sides, trunk/branch visibility, textures in daylight and shade, nearby and distant views, flat ground and slopes, walking around the trunk and up shallow roots. Record which exact MPQ hash was tested in which client.

If it fails, close the client and restore the previous patch. Do not modify unrelated UE code or game archives. Use controlled tests that change one factor at a time. Cache clearing was not the cure for our earlier format/material failures.

`T32` and `T64` are useful human labels for detail levels. They are not architecture detection and do not cause fallback. Our UE loader gives T64 priority over T32 when both custom archives are discovered, but that name pattern was not verified in stock WoW. The proof of concept retains the tested `patch-enGB-T.MPQ` filename. Separate `low-detail/` and `high-detail/` delivery folders can each contain that working filename; install only the intended variant.

## 8. Optional converter rebuild

```powershell
cmake -S converter-source -B converter-build
cmake --build converter-build --config Release
ctest --test-dir converter-build -C Release --output-on-failure
```

Use a Visual Studio C++ build environment and CMake 3.20 or later. If replacing `tools/OBJtoM2.exe` with a new build, rerun the toolkit and converter tests and record its new hash. The bundled binary's architecture describes the conversion tool, not a special 32-bit/64-bit asset format.
