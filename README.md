# OBJ-to-M2 Toolkit - Static Models

Convert general static, triangulated OBJ models to WoW 3.3.5a M2, SKIN, BLP2 and additive MPQ archives. Optional collision, opaque or alpha textures, and explicit transforms are supported. Blender is not required.

**Original OBJtoM2 converter: Garthog**, with the original contributors acknowledged in [CREDITS.md](obj-to-m2-toolkit/CREDITS.md). This project evolved from that converter. The current export pipeline uses a later Python writer; the user is not the sole author of the underlying tools or historical workflow.

## Start here

- [Toolkit documentation](obj-to-m2-toolkit/README.md)
- [AI handoff](obj-to-m2-toolkit/AI_INSTRUCTIONS.md)
- [Conversion guide](obj-to-m2-toolkit/docs/GUIDE.md)
- [Compatibility and migration](obj-to-m2-toolkit/docs/COMPATIBILITY.md)
- [Credits](obj-to-m2-toolkit/CREDITS.md) and [third-party notices](obj-to-m2-toolkit/THIRD_PARTY_NOTICES.md)
- [Existing example models and MPQ](obj-to-m2-toolkit/proof-of-concept/README.md)

## Quick start

Windows and Python 3.14 were tested. From the repository root:

```powershell
cd obj-to-m2-toolkit
python -m pip install -r requirements.txt
python scripts/pipeline.py build examples/demo.json --out builds/demo-001
python tests/test_static.py
```

Use a fresh output directory. Original source files and game archives are not modified. Builds are not installed automatically.

The five-tree example archive and unpacked assets remain unchanged as proof of concept. The conversion core has no tree-specific functions. Animated models belong in the separate WoW Model Toolkit.

Current limits include one texture/material and one SKIN section per model, 21,845 triangles and 65,535 exported vertices. New assets still need testing in the target game client. See the documentation for full scope and migration details.

Retired converter source, executable and helper files are preserved under [legacy/](legacy/README.md), with original content and credits intact.
