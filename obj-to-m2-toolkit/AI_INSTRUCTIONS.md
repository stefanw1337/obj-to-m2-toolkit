# AI handoff - OBJ-to-M2 Toolkit for Static Models

Read README.md and CREDITS.md first. Deliver code, comments, output messages and documentation in
English; converse with the user in Norwegian. This file does not authorize unrelated publishing,
communications, server changes or game-client changes.

## Scope

General static OBJ models for WoW 3.3.5a: props, furniture, rocks, vegetation, statues and decorations.
Do not introduce asset-specific deformation, collision fitting, presets or assumptions into the core.
The user explicitly allows the existing tree assets and MPQ as proof-of-concept examples. Preserve
proof-of-concept/ unchanged, including its historical metadata; it is not a current build template.

This current version uses a Python M2 writer. It no longer executes OBJtoM2.exe. The original converter
was credited to Garthog, not to the user or the upstream mirror owner. The toolkit evolved from that
converter and the retained proof-of-concept outputs were produced using the earlier workflow. Keep
that credit visible, distinguish subsequent modifications, and preserve upstream notices.

## Implementation

- scripts/pipeline.py: OBJ parsing, explicit transforms, normal/UV splitting, collision selection,
  multi-model builds, inspection, measurement and stage/MPQ validation.
- scripts/m2_writer.py: MD20 v264 and one SKIN profile.
- scripts/assets.py: opaque/alpha BLP2 textures and additive MPQ packing.
- scripts/validate.py: new-format structural and pose validation.
- scripts/legacy_validation.py: validation of previously exported static examples.
- tools/MPQEditor.exe: archive writer; separate third-party credit applies.

Blender is NOT required. Tested with Python 3.14 and requirements.txt. Inputs need triangulated faces
and UVs; missing normals become flat normals. Negative OBJ indices are supported. V is flipped by
default to match M2. Planar geometry is valid without box collision.

Collision modes: none (default), box, mesh (same transform as visible source), prepared_obj (already
in final output coordinates). No mandatory collision. Texture blend modes: opaque, cutout, alpha,
additive. Source texture sizes are preserved; require power-of-two dimensions no larger than 4096.

Transforms: convert Y/Z-up to Z-up, subtract origin, apply positive scale, rotate around Z, add offset.
Legacy geometry-specific parameters and texture overrides are rejected rather than silently applied.
Explain migration using current generic settings. Builds require fresh output directories and never
edit inputs or install patches automatically.

## Commands

```powershell
python tests/test_static.py
python scripts/pipeline.py build examples/demo.json --out builds/demo-002
python scripts/pipeline.py verify proof-of-concept/patch-enGB-T.MPQ proof-of-concept/archive
python scripts/pipeline.py pack proof-of-concept/archive builds/repacked.MPQ
```

Tests cover no collision, box collision, supplied collision in source/final coordinates, planar alpha
geometry, UV orientation, invalid input, negative indices and MPQ extraction. The 15 members in the
existing example MPQ were verified byte-for-byte. Do not infer that every newly exported model is
tested in-game from those checks.

## Locations and limits

Installed root: E:/ZOMBIE/obj-to-m2-toolkit/obj-to-m2-toolkit.
Previous version: E:/ZOMBIE/obj-to-m2-toolkit-backup-20261003. Preserve this backup.
Companion animated toolkit: E:/ZOMBIE/WoW-Model-Toolkit; read its separate handoff for zombie work.

Limits: one diffuse texture/material and one SKIN section per model, at most 21,845 triangles and
65,535 exported vertices. Splitting for UV seams/hard normals can increase the vertex count. Large
buildings requiring portals/interior systems need a WMO workflow, not a promise that M2 supports
every static world asset. Do not add animated conversion to the static CLI by flattening a rig to OBJ.

Keep source assets and synced project sources/ read-only. Preserve original game MPQs. User-authorized
test archives should be separate additions. Refresh SHA256SUMS.json and the ZIP after release changes,
exclude __pycache__, and keep credits visible in both the README and CREDITS.md.
