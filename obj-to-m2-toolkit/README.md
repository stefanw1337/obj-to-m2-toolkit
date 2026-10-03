# OBJ-to-M2 Toolkit - Static Models

Start here: [AI handoff](AI_INSTRUCTIONS.md) | [Credits and project lineage](CREDITS.md).

Original OBJtoM2 converter credit: **Garthog**, with the original contributors listed in CREDITS.md. The current Python writer is a later implementation; the project is not entirely original work by the user.

Convert static, triangulated OBJ models into World of Warcraft 3.3.5a M2 version 264,
SKIN, BLP2 textures and an additive MPQ. Suitable for props, furniture, rocks, vegetation,
statues, decorations and other static geometry. No asset-specific deformation or collision
fitting is applied. Blender is not required for this static conversion workflow.

## Setup

Windows and Python 3.14 were used for validation.

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe scripts/pipeline.py build examples/demo.json --out builds/demo-001
```

`setup.ps1` performs the two setup steps. MPQEditor is supplied for local archive creation;
see `THIRD_PARTY_NOTICES.md`. Inputs and existing MPQs are never changed by a build.
Use a new output directory each time.

## Configure any static model

Copy `examples/static-model-template.json`, set the OBJ and diffuse texture paths, and
choose unique archive paths. Input paths are relative to the JSON configuration.
Multiple model entries can be combined into one MPQ. Each model uses one texture/material.

- `up_axis`: `Y` or `Z`; output is Z-up.
- `origin`: point to subtract after converting the source coordinates to Z-up.
- `scale`: positive scalar or three positive axis scales.
- `rotation_z_degrees`: optional rotation after scaling.
- `offset`: final translation in output coordinates.
- `flip_v`: true by default to convert OBJ UV coordinates to M2 convention.
- Normals are preserved when supplied; missing normals become flat face normals.
- Planar models are valid when collision is disabled.

## Optional collision

- `none`: default; no collision triangles are generated.
- `box`: a generic axis-aligned box around the transformed visible geometry.
- `mesh`: a supplied triangulated collision OBJ, transformed exactly like the visible mesh.
- `prepared_obj`: a supplied collision OBJ already in final output coordinates.

No automatic shape assumptions are made. A detailed model can use a deliberately simpler
collision mesh. A planar model cannot use `box` collision because a closed box needs
nonzero dimensions on every axis.

## Materials and textures

`material.blend` supports `opaque`, `cutout`, `alpha`, or `additive`. Optional `two_sided`
and `unlit` flags are available. Images must have power-of-two dimensions, at most 4096.
Their size is preserved and full mip chains are generated. Opaque surfaces use BC1;
alpha surfaces use BC3. Transparency is not silently discarded in alpha modes.

## Commands

```powershell
python scripts/pipeline.py inspect model.obj
python scripts/pipeline.py build model.json --out builds/model-001
python scripts/pipeline.py measure builds/model-001/archive/World/Custom/Model.m2
python scripts/pipeline.py validate builds/model-001/archive
python scripts/pipeline.py verify builds/model-001/patch-S.MPQ builds/model-001/archive
python scripts/pipeline.py pack builds/model-001/archive builds/repacked.MPQ
python scripts/pipeline.py blp diffuse.png diffuse.blp --blend cutout
python tests/test_static.py
```

## Working example archives

The existing five-tree proof of concept is retained unchanged in `proof-of-concept/`,
including its MPQ, unpacked assets and historical validation reports. These are examples
of static models, not special conversion modes. To verify or repack the existing assets:

```powershell
python scripts/pipeline.py verify proof-of-concept/patch-enGB-T.MPQ proof-of-concept/archive
python scripts/pipeline.py pack proof-of-concept/archive builds/example-repacked.MPQ
```

The original source models are not bundled, so these commands repack the already converted
assets. Historical fitting notes describe that example's provenance and are not current
configuration templates. Model generation now uses the generic options above.

## Compatibility and scope

The previous `models` configuration structure and command names are retained. Replace
old geometry-specific options with explicit `offset` and a generic collision choice.
Use source image dimensions instead of the old `texture_size` override. The old special
dark-pixel adjustment is not part of the default pipeline; provide a prepared texture
if another renderer needs color corrections.

Current limits: 21,845 triangles and 65,535 exported vertices per model, one texture/material
and one SKIN section. UV seams and hard edges can increase exported vertex count. Models
must be triangulated, with UVs on every visible face corner. Concave polygon triangulation,
PBR material conversion, interior/portal systems and WMO export are outside this toolkit.
Large buildings can require a WMO workflow even though their geometry is static.

Animated NPCs belong in the companion WoW Model Toolkit. The user confirmed its Northshire
zombie MPQ working in WoW on 2026-10-03. That confirmation does not establish runtime
compatibility for every new static asset. Test new MPQs in the target client.

Build reports cover model structure, sampled static pose reconstruction, collision data
and byte-exact MPQ extraction. Install a generated MPQ as a separate archive; never replace
an original game archive. This toolkit does not automatically install builds.
