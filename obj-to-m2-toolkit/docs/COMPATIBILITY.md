# Compatibility and troubleshooting

## Converter fixes versus asset fixes

The bundled converter snapshot is from the user's fork at commit `eedf5a6c9f57b79c295ef12c80c40bc8edc15a0a`. Its README explains the upstream geometry errors: attributes were associated with position indices rather than full OBJ position/UV/normal tuples, and group ranges did not match repacked output geometry. The fork preserves seams and normals, adds separate collision, checks limits and calculates model/submesh/static sequence bounds.

These changes are not proof that every invisible-model failure was caused by the same bug. Later texture and renderer problems were isolated separately. Experimental NUL-name, alignment and SKIN-header rewrites were not established as the cure and are not part of this recipe.

## UE dark-pixel holes

Read-only inspection of the specific UE client found that paths containing `treecanopy`, `canopylesstree`, `treefacade` and some foliage names select a foliage material. That material multiplies texture alpha by a chroma mask which discards pixels when linear R+G+B is at or below 0.045. This can cut holes in opaque bark. The small Mid01 tree did not take the same route, explaining why it remained intact.

The optional `ue_dark_pixel_guard` keeps BC1 in opaque four-color mode and raises only dark block endpoints until the minimum used palette color has linear R+G+B >= 0.065 at every mip. This safety margin matched the successful asset-side experiment. It changes some texture colors, including potentially dark leaves, not just bark. It is specific to that renderer/material behavior; do not apply it blindly to unrelated clients. No UE source changes are included.

The shader can still shade trunks very dark because of source color, normals, foliage material properties or shadows. Visible sunlight on the canopy proves some light response, not that trunk lighting is ideal. Do not switch to unlit/emissive merely to hide a shading issue.

## Texture size and encoding

The current baseline is opaque BLP2 BC1/DXT1, 2048 square, with all mip levels. The user reported 1024 and 2048 test variants working in stock WoW. Earlier 4096 variants did not work in those tests, but not every experiment held M2 metadata constant. This does not prove a universal hardware/client maximum. An all-opaque DXT3 diagnostic also failed in stock WoW and was abandoned.

BC1 four-color normalization alone did not fix UE trunk holes. The source images were opaque, and alpha checks were not evidence that RGB-based masking could not occur. Check the exact shader interpretation when diagnosing similar symptoms.

## Trunk placement and collision

Root geometry is fixed to the model; it does not bend to terrain automatically. Extend undersides below the local ground plane or lower the model modestly, then test slopes and flat placements. A large flat root apron may need actual remodeling. Polar collision proxies bridge gaps and can create walkable surfaces across root gaps; inspect them before accepting the approximation.

Our latest extreme oak was nonuniformly stretched to roughly match Canopy04's reference height and crown width. That increased the apparent height of the root mass. It was subsequently lowered another two model units. This is recorded as a pending visual test, not a universal transform to apply to other oaks.

## Loading and diagnosis

If originals remain visible, check replacement paths, patch installation and priority first. If trees disappear, verify complete M2/SKIN/BLP sets and member references; isolate geometry and texture changes. If collision remains while rendering fails, that does not establish which rendering component failed. If only one client fails, compare its interpretation without changing the other client blindly.

Use one controlled custom patch during tests. Keep backups outside Data, because a backup MPQ in a scanned location can still override files. `patch-enGB-T.MPQ` is the name tested in this workflow. T32/T64 naming was checked in the UE loader but not in stock WoW. Higher detail is not an architecture tag in M2.

## Scope limits

Static, single-texture, opaque props only. No animated trees, wind rigs, automatic LOD generation, material atlas baking, foliage alpha-cutout setup, arbitrary legacy M2 importing, terrain placement editing or automatic client-specific asset selection. The validator covers this kit's outputs, not every valid variant of the WoW formats.
