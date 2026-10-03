# Third-party notices

This toolkit includes project-specific work and third-party tools. Original authorship and applicable notices remain distinct; no new blanket license is assigned.

## MPQEditor

The archive utility in tools/MPQEditor.exe is the user's existing copy of MPQEditor by Ladislav Zezula: https://www.zezula.net/en/mpq/download.html . Its original terms apply. Public redistribution rights for this executable have not been established. Prefer the official download for public distribution.

## Runtime dependencies

Blender, Python, NumPy, Pillow and mpyq are external dependencies with their respective licenses. They are not bundled. The Python modules in scripts implement the current export pipeline. The legacy OBJtoM2 executable and source are not required or included in the curated release ZIP.

## References and assets

Format references include wowdev/pywowlib, AzerothCore's animation definitions, and the user's existing format reference sources. No blanket license is assigned to reference code, game assets, or third-party tools.

The separate Northshire client-test MPQ contains the user's extracted D3 model/texture and DBC overlays based on the user's WoW client. It is a local test artifact. Original extracted assets and full original game archives are not included in the toolkit ZIP. Asset rights and public redistribution permissions are outside this local build.

## Original converter attribution

The earlier OBJtoM2-based workflow credits Garthog and its original contributors. See [CREDITS.md](CREDITS.md) for provenance and the distinction between that converter and this release's Python implementation. Preserve these credits with future releases.
