# PiT Launcher

A launcher for the European release of *Mario & Luigi: Partners in Time*
(`ARMP`, SHA-1 `ba4ec2f99b4f2e0047601552bccf00aa73e28701`).

Your own EUR cartridge is the only source of the game. The launcher verifies
that the supplied file really is that ROM, pre-checks the stat table through
the NitroFS directory tree, prepares a copy for the decompilation project in
this repository, and exports the generated data the project needs — the
cartridge banner as a PNG, the NitroFS file tree, a probe of
`sound_data.sdat`, and a manifest — next to the copy. No Nintendo content is
downloaded, shipped or stored in this tool or its APK.

## Hard Mode is optional

The **MODS** tab lists the mods you can apply, **none by default**. The first two
rows are always `NO MOD` and **Hard Mode**; any folder you drop into `mods\` is
listed after them. Pick **Hard Mode** if you want enemy HP, power, defense and
speed raised, along with experience and coin drops:

| Field | Scale |
|---|---|
| Enemy HP | x11/10 |
| Enemy power | x2 |
| Enemy defense | x21/20 |
| Enemy speed | x5/4 |
| Experience | x7/4 |
| Coins | x7/4 |

With no mod selected, the launcher does not edit the copy beyond repairing its
header CRC; the output is suffixed `.prepared.nds`. With a mod selected, the 98
stat records are scaled and the output name carries that mod's own suffix, such
as `.hardmode.nds`. The choice is remembered on the screen and re-applied to the
output file name, so you can never mistake one prepared copy for another.

### Your own mods

A mod is a folder under `mods\` containing a `profile.json`:

```json
{
  "schema": "pit-mod-profile-v1",
  "id": "my_mod",
  "name": "My Mod",
  "version": "1.0.0",
  "description": "Shown under the list.",
  "transforms": [
    { "field": "max_hp", "scale": 1.1, "min": 1 },
    { "field": "coins", "scale": 1.75, "enabled": false }
  ]
}
```

A profile may only scale the six stat fields the launcher already owns, so it
cannot write outside the record it is editing. `scale` is a decimal number or a
fraction, `min` and `max` clamp the result, and `"enabled": false` keeps a
transform visible in the list while leaving it out of the patch. Two rules keep
a profile unambiguous, and both launchers enforce them the same way:

- the `id` must be 1 to 32 characters and is **refused** if longer, because it
  identifies the profile for `--mod`, the list and the output name;
- one field may appear only once, because scaling it twice would make the
  written bytes depend on the order you happened to write them in.

An over-long `name`, `version` or `description` is truncated rather than
refused: those are only ever shown on one clipped line, and the shipped Hard
Mode description is longer than the buffer holds.

The headless driver `pit_patch.exe` takes flags for the same list, which is
handy for checking a profile without opening the launcher:

| Flag | Effect |
|---|---|
| `--no-mods` | verify and copy with no mod applied |
| `--mods-dir <dir>` | where profiles are discovered (default `mods`) |
| `--mod <id>` | apply this profile instead of the built-in one |
| `--list-mods` | print the catalog and exit |
| `--ingest <dir>` | also export generated data (banner, NitroFS tree, SDAT probe, manifest) |

The Java desktop harness `PitSelfTest` adds `--in-place`, a testing aid that
reproduces the Android app's single-buffer memory behaviour so both can be
checked against each other.

## What you need

Your own copy of the European cartridge ROM. It is not included, and no part of
it is stored in this tool or its APK.

## Builds

| Platform | Artifact | How |
|---|---|---|
| Windows x64 | `pit_launcher.exe` | CMake, or `build-release.ps1` to package a ZIP |
| Windows x64 | `pit_patch.exe` | headless driver, same pipeline |
| Android | `pit-patcher-release.apk` | `android\build-apk.ps1` (Windows) or `android\build-apk.sh` (POSIX) |

Both are built by CI on every push, and the artifacts are attached to the run:

| Workflow | Produces |
|---|---|
| `.github/workflows/windows.yml` | `PiT-Launcher-win64.zip`, plus the rendered UI frames |
| `.github/workflows/android.yml` | `pit-patcher-release.apk`, or an unsigned APK when no signing secret is set |

The Windows build needs CMake, Ninja, a C compiler and git; SDL2 is fetched and
built statically from the tag pinned in `CMakeLists.txt`, so the result is a
single EXE with no runtime to install. The Android build needs a JDK 17 and the
Android SDK build-tools; it has no Gradle, NDK or native code, and requests no
storage permission — files are chosen through the Storage Access Framework.

To sign a release APK, set `ANDROID_KEYSTORE` to a base64 keystore,
`ANDROID_KEYSTORE_PASSWORD` and `ANDROID_KEY_ALIAS` as repository secrets. The
key itself is never committed, and the build script writes it under `build/`,
which is ignored.

## Using it

The launcher has four tabs — **HOME**, **MODS**, **SETTINGS** and **ABOUT** —
driven by mouse, touch and the keyboard.

1. Choose your EUR ROM on **HOME** (or drop it onto the window).
2. Choose an output file. The output must not be the source.
3. Optionally open **MODS** and pick a mod.
4. Press **PREPARE ROM**.

The launcher verifies the ROM size, SHA-1, title, game code and header CRC-16,
locates `BData/BDataMon.dat` through the NitroFS directory table, and — with a
mod selected — scales the 98 stat records and recomputes the header CRC. It
always works on a copy, so a failure never damages the source. **SETTINGS**
holds the theme, window and mods-folder options, remembered in
`%APPDATA%\PiT-Launcher\config.ini`.

## Important

**A prepared ROM is not expected to boot yet.** The decompilation in this
repository is incomplete: there is no bootable reconstruction, so nothing in
this project can run the game today. The launcher demonstrates a working,
verified pipeline: real cartridge data is verified and passed to the
decompilation, nothing more. No Nintendo content is shipped, extracted or
committed here.

## Layout

```
src/core/            patch pipeline, ROM/FNT reader, SHA-1, PNG writer, data export
src/ui/              Dear ImGui launcher (SDL2 + SDL_Renderer)
src/platform/sdl2/   SDL2 platform layer for the launcher and runtime
tools/               plan generator, font generator and verification scripts
android/             pure-Java patcher, same generated plan
mods/                mod profiles, one folder per mod
```

`tools/patch_sources/hard_mode_plan.json` and `tools/patch_sources/font8x8.txt`
are the only inputs. `tools/gen_patchplan.py` generates the C header and Java
class both builds compile against, so the two cannot drift apart.

`mods/hard_mode/profile.json` states the built-in mod's multipliers in the same
form a mod author would write, so the shipped mod is read by the same parser as
any other rather than being special-cased.

The 5x7 font is generated by `tools/make_font.py` from the shared text table;
the Android patcher blits it as rectangles, so no font file is shipped there.
The Windows launcher uses a Dear ImGui face instead (Segoe UI when present, the
built-in face otherwise).

## Verification

Everything that does not need a cartridge:

```powershell
./check.ps1
```

That covers the generated plan being current, the font's 95 glyphs and their
byte-identical C and Java tables, the absence of ROM data in the tree, the Java
patcher compiling warning-free under `-Xlint:all -Werror`, 1081 unit assertions,
and four headlessly rendered launcher frames checked for size and theme colours
(the HOME, MODS, SETTINGS and ABOUT tabs).

Checks that do need your own EUR ROM, and are therefore not part of CI:

```powershell
python tools/check_patch_roundtrip.py --rom <rom.nds> --export <export.json>
python tools/check_c_java_parity.py --rom <rom.nds>
```

The last one patches the same ROM with both the C and the Java build and
compares the results byte for byte, with the hard-mode mod both on and off.
Current result: **exact match** in both modes, 98 records and 588 fields.

### Render check

`tools/check_launcher.py` loads a BMP the launcher wrote with `--screenshot` and
asserts it is a 1200x760 24-bit image carrying the theme's signature colours, so
a regressed theme or a frame that failed to draw fails the build rather than
shipping.

## Legal

This is a fan tool for a game you must own. It contains no Nintendo code, ROM
data, extracted assets or trademarks. Do not redistribute the ROM.