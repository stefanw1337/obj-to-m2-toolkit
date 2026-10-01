# Third-party notices and publication status

This is a local handoff bundle assembled from the user's existing tools and custom outputs. It has not been published by this task. No blanket MIT/GPL/other license is assigned to the bundle, the user assets or third-party code.

## OBJtoM2

- Original code credits Garthog and thanks relaxok, schlumpf, gamh, mjollna, zim, pxr.dk and modcraft. Credits remain in the bundled source.
- Upstream mirror: https://github.com/bollafa/OBJtoM2 (its README says it was not made by the mirror owner).
- User fork: https://github.com/stefanw1337/OBJtoM2
- Snapshot: eedf5a6c9f57b79c295ef12c80c40bc8edc15a0a
- No explicit redistribution license was found in the inspected upstream repository or local snapshot. Public visibility/forkability is not equivalent to an unrestricted software license. Determine the actual rights before publishing a downloadable source/binary bundle. Do not relabel inherited code as MIT.

GitHub's explanation: https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository

## MPQEditor

MPQEditor by Ladislav Zezula is included as a copy of the user's local executable. Official download page: https://www.zezula.net/en/mpq/download.html . Public redistribution terms for this exact executable were not confirmed in this task. For a publicly shared toolkit, prefer directing recipients to the official download unless distribution rights are established. Do not assume the separate StormLib license automatically licenses MPQEditor.

## Python components and optional tools

Python itself and dependency wheels are not bundled. `setup.ps1` obtains the pinned NumPy, Pillow and mpyq packages from the user's configured pip index. Their upstream licenses apply. Optional Blender, CMake, Visual Studio and the Microsoft C++ runtime must be obtained separately under their own terms. Source scripts and documentation added for this workflow are distinct from inherited converter code; this bundle does not decide the user's publication license for them.

## Proof-of-concept assets and MPQ format

The proof of concept contains the user's converted tree models/textures and generated collision, not a copy of the original Blizzard tree meshes, texture images or game archives. Warcraft archive paths are used as replacement identifiers. Source ownership, any asset generator terms and redistribution permissions have not been independently audited.

Creating an archive in MPQ format is a technical operation; the filename or container alone does not settle whether a particular use or distribution is lawful. Rights in the contents, third-party software terms, client agreements and applicable law are separate questions. Blizzard's current terms are available at https://www.blizzard.com/en-us/legal/ . They should not be treated as evidence of approval for this project or assumed to be the exact historical agreement applicable to every 3.3.5 installation.

Before a public release, establish asset and tool redistribution rights and select an appropriate license only for material the user can license. This notice records what is known; it is not a legal clearance for the bundle.
