# Credits and project lineage

## Original OBJtoM2 converter

**Garthog** is credited as the original author in the OBJtoM2 source and executable's
startup message. The original acknowledgements thank **relaxok, schlumpf, gamh, mjollna,
zim, pxr.dk and modcraft** for code, advice and format knowledge.

- Upstream mirror: https://github.com/bollafa/OBJtoM2 . The mirror's owner is not identified
  as the original author; its README states that the program was not made by the mirror owner.
- User fork: https://github.com/stefanw1337/OBJtoM2 .
- Earlier inspected source snapshot: eedf5a6c9f57b79c295ef12c80c40bc8edc15a0a.

The original OBJ-to-M2 toolkit was built around this existing converter, with subsequent
compatibility fixes, conversion scripts, validation and archive packaging. It must not be
presented as entirely authored by the user or as an original converter invented in this chat.
The retained example MPQ and static assets were produced with that earlier workflow.

## Current toolkit changes

The current release uses a later Python M2/SKIN writer and static conversion pipeline,
developed with Codex assistance for this project. It does not execute or bundle OBJtoM2.exe.
This implementation change does not erase the project's origin or the original converter's
credit. Garthog is credited for OBJtoM2, not attributed authorship of every later Python file.

## Other software and references

- **Ladislav Zezula:** MPQEditor - https://www.zezula.net/en/mpq/download.html .
- **wowdev contributors:** M2 format documentation and pywowlib reference structures -
  https://wowdev.wiki/M2 and https://github.com/wowdev/pywowlib .
- **AzerothCore contributors:** animation definitions and server documentation -
  https://github.com/azerothcore/azerothcore-wotlk .
- **Python, NumPy, Pillow and mpyq contributors:** runtime and supporting libraries.

Credits identify contributions; they do not replace licenses. Preserve original notices
with any inherited source or binaries. See THIRD_PARTY_NOTICES.md for the bundle's recorded
third-party status. Do not assign the user's authorship or a new blanket license to others' work.
