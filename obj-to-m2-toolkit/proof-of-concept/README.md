# Five-tree proof of concept

`patch-enGB-T.MPQ` is the actual latest delivered five-tree archive, including the extra two-unit lowering of extreme oak. The unpacked members in `archive/` match it byte-for-byte. `inventory.json` lists paths, dimensions, geometry counts and hashes. `last-adjustment-validation.json` records the final lowering.

## Included replacements

All model paths below are under `World/Azeroth/Elwynn/PassiveDoodads/Trees/`.

| Source tree | Model | Triangles |
|---|---|---:|
| Small oak | ElwynnTreeMid01.m2 | 5,626 |
| Big oak | ElwynnTreeCanopy01.m2 | 5,497 |
| Medium oak | ElwynnTreeCanopy02.m2 | 5,623 |
| White oak less canopy | CanopylessTree01.m2 | 5,499 |
| Extreme oak | ElwynnTreeCanopy04.m2 | 19,801 |

There are 15 asset members: five M2 files, five matching `00.skin` files and five BLP textures. Textures are opaque BC1 at 2048 with complete mip chains. Trunk/root collision is included. The first two canopy trees retain their previous 0.6/0.4 unit lowering. Extreme oak retains the provisional tall/narrow fit and is lowered another 2 units relative to the first five-tree test. These offsets are model-space values and are multiplied by placement scale.

## Test status

- Earlier three-tree compatibility assets were reported working in both stock WoW 3.3.5a and the custom UE client after the dark-pixel fix. Collision was reported working during the earlier tests.
- A screenshot of the initial five-tree package showed extreme oak in the world and revealed an overly prominent root mass.
- This latest archive includes the subsequent extra two-unit lowering. Its member bytes and model data were verified, but user confirmation of that latest adjustment in both clients is still pending.
- The archive is a proof of the conversion/package workflow, not a claim that all five assets are visually finished or performance-certified.

## Reproduce the package

From the toolkit root:

```powershell
python scripts\pipeline.py verify proof-of-concept\patch-enGB-T.MPQ proof-of-concept\archive
python scripts\pipeline.py pack proof-of-concept\archive builds\poc-repacked\patch-enGB-T.MPQ
```

The original OBJ/Blender/JPG sources are not embedded in the toolkit. Repacking the supplied final members is reproducible without them; regenerating the five models from source requires the user's originals and their recorded transforms. `new-model-fit-before-final-lowering.json` is historical fit metadata for the two newest trees, not a ready-to-run configuration for rebuilding all five.

Use a separate test installation. Close clients, preserve the previous custom patch outside Data, and replace only the custom `Data/enGB/patch-enGB-T.MPQ`. Do not edit original archives. Undo by restoring the previous custom patch with clients closed.
