# OBJ to M2 Tree Toolkit

A local Windows handoff kit for converting **static, opaque, triangulated OBJ props** into WoW 3.3.5a M2 version 264, SKIN, BLP2 textures and a verified MPQ. Includes our five-tree replacement archive as a proof of concept.

This kit was assembled on 2026-10-01. It creates output files; it does not install patches, launch clients, modify an Unreal project, change original game archives, or publish to GitHub.

## Start here

- Human walkthrough: [Conversion guide](obj-to-m2-toolkit/docs/GUIDE.md)
- AI handoff: [AI instructions](obj-to-m2-toolkit/AI_INSTRUCTIONS.md)
- Known issues and tested fixes: [Compatibility notes](obj-to-m2-toolkit/docs/COMPATIBILITY.md)
- Example archive and test status: [Proof of concept](obj-to-m2-toolkit/proof-of-concept/README.md)
- Attribution and publication status: [Third-party notices](obj-to-m2-toolkit/THIRD_PARTY_NOTICES.md)

## Requirements

Windows, Python 3.14 (the version used for validation), and the packages in `requirements.txt`. The converter and MPQEditor executables are bundled for this local kit. Python itself is not bundled. Blender is optional for viewing models and generating previews; CMake and Visual Studio C++ tools are only needed to rebuild the converter. If the converter reports missing Microsoft runtime DLLs, install Microsoft's supported Visual C++ Redistributable from its official site.

From PowerShell in this folder:

```powershell
.\setup.ps1
.\.venv\Scripts\python.exe scripts\pipeline.py --help
```

`setup.ps1` creates a local virtual environment and downloads the pinned Python packages using pip. If script execution is restricted, run these commands directly instead of changing your system policy:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

Dependencies are pinned to the versions used here. Installation on a clean machine and availability of matching wheels are not verified by the included tests.

## Run a complete synthetic example

```powershell
.\.venv\Scripts\python.exe scripts\pipeline.py build examples\demo.json --out builds\demo-001
```

This generates a small test trunk with a 16px texture, its closed collision proxy, M2/SKIN files, an MPQ and validation reports. It uses `World/CustomTrees/DemoTrunk.m2`, which is not automatically placed in the WoW world. It is a tool smoke test, not another tree replacement.

## Verify or repack our actual proof of concept

```powershell
.\.venv\Scripts\python.exe scripts\pipeline.py verify proof-of-concept\patch-enGB-T.MPQ proof-of-concept\archive
.\.venv\Scripts\python.exe scripts\pipeline.py pack proof-of-concept\archive builds\repacked\patch-enGB-T.MPQ
```

Repacking preserves member bytes. MPQ container bytes may differ because of archive metadata or tool behavior. Compare extracted members and the generated reports rather than expecting identical container hashes after repacking.

## Convert a new tree

Copy `examples/tree-template.json` alongside your OBJ and base-color image, edit the file names, target archive paths, transforms and collision settings, then:

```powershell
.\.venv\Scripts\python.exe scripts\pipeline.py inspect "D:\MyTrees\oak.obj"
.\.venv\Scripts\python.exe scripts\pipeline.py build "D:\MyTrees\tree.json" --out builds\oak-001
```

Input file paths are relative to the configuration file. Output directories must be new. Multiple objects can be listed in `models` to create one combined MPQ. Read the guide before choosing transforms or model paths; a screenshot does not establish an exact world model path.

## Included tools

| Item | Purpose |
|---|---|
| `tools/OBJtoM2.exe` | Fixed Wrath converter, compiled from the bundled source snapshot |
| `tools/MPQEditor.exe` | Create MPQ archives |
| `scripts/pipeline.py` | Inspect, transform, convert, validate, pack and verify |
| `scripts/textures.py` | Opaque BLP2/BC1 mip chain and optional UE dark-pixel workaround |
| `scripts/new_tree_collision.py` | Sample a trunk/root collision proxy |
| `scripts/trunk_sections.py` | Cross-section helper |
| `converter-source/` | Converter source and its binary round-trip tests |
| `proof-of-concept/` | Latest MPQ, exact unpacked members, inventory and known test status |

The Python scripts are the reusable workflow. They do not depend on the original chat or any `E:\WowUnreal` path. Original Blender files, source tree assets, Blizzard models and base game archives are not included.

## Tests

```powershell
.\.venv\Scripts\python.exe tests\test_pipeline.py
.\.venv\Scripts\python.exe converter-source\tests\test_conversion.py tools\OBJtoM2.exe
```

File checks do not prove runtime rendering or gameplay behavior. Each new package needs testing in the actual target clients. This kit's validator intentionally accepts the restricted static/opaque output produced here; it is not a universal M2 or MPQ validator.
