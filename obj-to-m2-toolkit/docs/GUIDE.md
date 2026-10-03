# Static model conversion guide

1. Preserve the original OBJ and image. Work from a configuration beside the source assets and choose a fresh output directory.
2. Verify the desired archive model path from the target client. A screenshot alone does not identify a replacement path. A new path is not automatically placed in the world.
3. Export triangulated OBJ geometry with UVs on every face corner. Supply normals to preserve smooth shading; missing normals become flat face normals. Negative indices are supported.
4. Copy `examples/static-model-template.json`. Set the source paths and unique M2/BLP archive paths. Inputs are relative to the configuration file. Multiple model entries can share one output archive, using distinct output paths.
5. Choose `up_axis`, `origin`, `scale`, `rotation_z_degrees` and `offset` deliberately. The origin is expressed after Y/Z-up conversion. Scaling can be uniform or per axis; supplied normals receive the inverse-transpose transform.
6. Choose collision: `none`, `box`, `mesh` or `prepared_obj`. A `mesh` collision OBJ receives the visible model transform; a `prepared_obj` is already in final output coordinates. Collision is optional, and a flat model does not need artificial height.
7. Select the material blend mode: `opaque`, `cutout`, `alpha` or `additive`. Use `two_sided` or `unlit` only when intended. Prepare one diffuse atlas per model; PBR graphs are not imported. Image dimensions must be powers of two and at most 4096.
8. Run the build, then inspect its validation report and MPQ members. Geometry and file checks do not establish gameplay collision or visual quality.
9. Test a new additive MPQ in the target client, keeping original game archives intact. Record the exact archive hash and the observed results.

```powershell
python scripts/pipeline.py inspect model.obj
python scripts/pipeline.py build model.json --out builds/model-001
python scripts/pipeline.py validate builds/model-001/archive
python scripts/pipeline.py verify builds/model-001/patch-S.MPQ builds/model-001/archive
```

The SKIN name appends `00.skin` to the complete M2 stem. Keep the M2, corresponding SKIN and referenced BLP together. Texture V is flipped by default; `flip_v: false` is available for inputs already using the required convention.

See [compatibility notes](COMPATIBILITY.md) for changes from the earlier toolkit and [the README](../README.md) for format limits.
