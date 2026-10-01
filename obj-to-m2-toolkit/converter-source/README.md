# OBJtoM2
Standalone OBJ to World of Warcraft **Wrath (M2 version 264)** converter.
The original converter is credited to Garthog in the program; this repository
contains fixes to its OBJ geometry conversion. It does not depend on Blender.

Forked from [bollafa/OBJtoM2](https://github.com/bollafa/OBJtoM2).
The original author credits are preserved; this fork extends the existing work.

## Why this fork exists

The goal is a standalone, repeatable conversion path for static custom props in
WoW 3.3.5a, including trees with collision limited to their trunks and roots.

The original OBJ conversion keyed output vertices only by position index. OBJ
allows the same position to have different UV and normal indices on adjacent
faces. The original loop retained the first attributes for that position, losing
UV seams and hard normals. This fork keys vertices by the complete `(v, vt, vn)`
tuple within each section and rewrites triangle indices accordingly.

The original group handling calculated section sizes from sets of source indices
without repacking vertices into contiguous section ranges. It could also omit
faces before the first group and count repeated triangles differently from the
triangle buffer. This fork builds matching contiguous section ranges and retains
all faces. Regression cases exercise these behaviors independently of the writer.

Additional changes provide explicit texture paths, optional separate collision
geometry, input validation, modern CMake builds, and binary round-trip tests.
Section bounds are calculated from visible geometry instead of a fixed radius.
The generated static sequence now has a nonzero selection weight and bounds
matching the visible model. These sequence changes are defensive corrections;
they were not proven to be the cause of the earlier invisible-tree symptom.

## Build on Windows

Install Visual Studio with the Desktop development with C++ workload and CMake.
From the repository directory:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Tests require Python 3. The executable is `build/Release/OBJtoM2.exe`.
The original `.vcxproj` targets the old v110 toolset; use CMake for modern builds.

## Convert a static prop with one texture

```powershell
.\build\Release\OBJtoM2.exe "oak.obj" "oak" --texture 'World\CustomTrees\oak.blp'
```

This writes `oak.m2` and `oak00.skin`. The output argument is a filename stem,
without `.m2`; its parent directory must already exist. `--texture` is the
**internal game archive path**, not the path to a local JPG or PNG. This mode
assigns the same opaque, single-sided material to all sections. It does not
read MTL files or convert images. Prepare the BLP texture separately and pack it
at exactly that archive path.

Omit `--texture` to use the original interactive material commands (`te`, `r`,
`tu`, `i`, `q`). Use `q` to save; an unexpected end of input aborts without saving.

## Separate trunk collision

```powershell
.\build\Release\OBJtoM2.exe "oak.obj" "oak" --texture 'World\CustomTrees\oak.blp' --collision "oak_trunk.obj"
```

`--collision` replaces full-mesh collision with a separate triangulated OBJ.
Its vertices and faces are used only for collision; UVs and normals are not
required. Face normals are calculated during export. Empty meshes, invalid
indices and degenerate triangles are rejected before saving. Author a closed
proxy with outward-facing triangles, in the **same coordinates and scale** as
the visible OBJ. For trees, omit foliage from that proxy. Without this option,
the original full-mesh collision behavior remains.

The converter does not silently rotate or scale either OBJ. Prepare both in
WoW model space (Z up) with the intended foot position at the origin. For a
Y-up source, the rotation `(x,y,z) -> (x,-z,y)` preserves handedness. Apply it to
both geometry and normals; scale both visible and collision meshes together.

## OBJ requirements and fixes

- Triangular faces with position, UV and normal indices (`v/vt/vn`) are required.
  Export UVs and normals and triangulate in your modeling application first.
- Whitespace, comments and negative relative indices are supported. Invalid
  indices, missing attributes and non-triangular faces produce an error.
- Export vertices are keyed by the complete position/UV/normal index tuple,
  preserving UV seams and hard normals. Shared positions can therefore produce
  more M2 vertices without increasing triangle count.
- Each nonempty group/material section has contiguous vertices and triangles.
  Faces before the first group and repeated faces are retained.
- UV V is flipped once, matching the original converter's convention.
- This writer conservatively accepts at most **65,535 exported vertices and
  65,535 triangle indices (21,845 triangles)**. Larger meshes are rejected, not
  truncated. These are limits of this implementation, not a universal M2 or UE
  polygon budget. Supporting larger meshes needs additional section handling.

## Validation and remaining limitations

The regression suite independently parses emitted M2/SKIN files and verifies
seams, normals, section ranges, index validity, separate collision, static
sequence bounds and rejected inputs. The current suite contains 13 tests.

This is an experimental static-model converter, not a full audited M2 exporter.
The writer generates collision from **the entire visible mesh** unless
`--collision` is supplied. Section centers/radii now come from the visible
geometry, while collision bounds come from the selected collision mesh.
Regression tests verify that these remain independent. It does not export wind
animation, modern PBR materials, or automatically adjust scale and axes.
A successful conversion is not proof of correct appearance in a game.

Preview the model and validate scale, orientation, materials, bounds and collision
in the target client before deploying an MPQ replacement. No game data is modified
by the conversion command itself.

## Client validation

On 2026-09-30, the asset author reported successful rendering and collision in a
WoW 3.3.5a test client using three custom oak assets, direct M2/SKIN outputs from
this converter, separate trunk/root collision, and 2048 x 2048 BLP2 DXT1 textures.
The final validation used the converter's own files, not the original-model
metadata used in intermediate diagnostic builds. This validates that tested
static-prop workflow, not every possible OBJ, material, or client configuration.

Earlier tests with 4096 x 4096 textures produced invisible trees. Geometry with
native textures rendered successfully; 1024 and 2048 texture variants also
rendered. Some intermediate tests changed model metadata as well, so the evidence
does **not** establish a universal 2048 client limit or prove that the upstream
OBJ importer caused the texture-loading failure. Use 2048 for the validated
workflow and test larger textures separately with identical M2/SKIN files.
Experimental alignment, string-length and SKIN-profile rewrites used during
diagnosis are not included as purported fixes in this fork.

Terrain fit remains asset-specific: some roots can float on slopes. Adjust the
source geometry, origin or placement and keep collision aligned. Larger variants
must scale visible geometry and collision together; existing world-placement
scales apply on top. A model-path replacement affects every placement using it.

## BLP textures and patch packaging

Texture conversion and MPQ creation are separate from OBJtoM2:

1. Preserve the original OBJ, MTL, source images and Blender files. Work on copies.
2. Export a triangulated OBJ with UVs and normals, oriented Z-up and scaled to the
   target prop. Prepare a separate collision OBJ in the same coordinate system.
3. Convert the base-color image to BLP2 DXT1 for an opaque asset. The validated
   setup uses 2048 x 2048 with a complete mip chain down to 1 x 1. Assets needing
   alpha require an appropriate texture format and material setup; the simple
   `--texture` mode is opaque and does not configure foliage cutouts.
4. Pass the intended internal BLP archive path to `--texture`. Keep the generated
   M2 and its matching `00.skin` together. For example, `ExampleTree.m2` needs
   `ExampleTree00.skin` at the same internal directory.
5. In an MPQ editor, create a separate test patch. Put the M2/SKIN at the original
   model's archive path and the BLP at the exact path supplied to `--texture`.
   Verify the packed files by extracting or reading them back.
6. Close the test client before replacing its patch. Test visibility, textures,
   scale and collision in-game, including on slopes. Retain a working patch for
   comparison and rollback; leave the original game archives unchanged.

The tested enGB installation used `Data/enGB/patch-enGB-T.MPQ`. This is a tested
installation choice, not an automatic deployment feature of the converter.
The repository does not include game archives, extracted Blizzard assets, or the
custom oak models and texture test packages. GitHub documentation and commit
messages for this fork are written in English.
