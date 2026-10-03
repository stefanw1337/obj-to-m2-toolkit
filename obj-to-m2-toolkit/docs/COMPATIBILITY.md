# Compatibility, migration and troubleshooting

## Current implementation

The active pipeline uses a Python M2/SKIN writer. The earlier Garthog-derived OBJtoM2 executable and source remain accessible in Git history and the user's preserved local backup; they are not part of the current runtime. Attribution remains in CREDITS.md and PROVENANCE.json.

The `models` configuration list and inspect/build/measure/validate/verify/pack/blp command names remain. Blender is not required. The new example is a generic static prop. Old asset-specific collision fitting has been removed; choose none, box, mesh or prepared_obj instead.

Replace `lower_z` with an explicit negative Z `offset`. Automatic `extend_foot` deformation is no longer provided; make geometry changes in the source editor. Legacy texture_size and ue_dark_pixel_guard options are rejected: prepare the source image at the desired resolution and color values instead.

## Textures and historical renderer behavior

Opaque textures use BC1; alpha surfaces use BC3. A complete mip chain is generated. Four blend modes are supported, but PBR effects and multiple material slots are outside this release.

Earlier tests reported 1024 and 2048 textures working in stock WoW. Earlier 4096 experiments failed without proving a universal client limit. This exporter accepts up to 4096; test the actual target client rather than treating acceptance as runtime certification. An earlier opaque DXT3 diagnostic also failed; this release does not use that encoding.

The preserved example assets include a prior color adjustment for a particular UE foliage material that discarded dark pixels through an RGB mask despite opaque alpha. This was renderer-specific, not a general M2 requirement. Preserve that historical evidence without reintroducing it as a default operation for all static models. Do not change an unrelated renderer or use unlit materials merely to hide a shading problem.

## Validation and loading

The current binary validator covers the writer's supported subset. A separate legacy validator permits verification/repacking of the unchanged example archive. Neither is a universal importer for arbitrary M2 files.

If the old model remains visible, check archive priority and exact member paths. If the new model disappears, verify the M2/SKIN/BLP set and internal references. Archive overlays replace matching members, not every member of an earlier archive; unrelated entries remain available. Keep backup MPQs outside scanned Data directories.

The historical proof-of-concept reports retain their original test status. No runtime success for a new static asset should be inferred solely from example success, a Blender render or a successful archive round trip.

## Scope

At most 21,845 triangles, 65,535 exported vertices and one texture/material and SKIN section per model. UV seams and normals may split source vertices. Input faces must be triangulated. Large buildings with interiors/portals may require WMO. Animated assets use the companion toolkit, not an OBJ conversion that discards the rig.
