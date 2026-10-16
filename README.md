# MOTHER 3 Recomp

MOTHER 3 for Windows, Linux and Android, using gbarecomp and recomp-ui.
Provide your own ROM and GBA BIOS; game data is not included.

## Downloads

| Platform | Current public version | Download |
| --- | --- | --- |
| Windows x64 | 1.0.4 | [Windows ZIP](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/download/v1.0.4/Mother3Recomp-v1.0.4-windows-x64.zip) |
| Linux x86_64 | 1.0.4 | [Linux archive](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/download/v1.0.4/Mother3Recomp-v1.0.4-linux-x86_64.tar.gz) |
| Android ARM64 | 1.0.5 | [Android APK](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/download/v1.0.5/Mother3Recomp-v1.0.5-android.apk) |

The 1.0.6 fixes in this branch are candidates awaiting device validation.
Reports about v1.0.5 adaptive rendering and fresh installation are tracked in issue #9.

## Playing

Extract the Windows ZIP and run `Mother3Recomp.exe`. Select your ROM and BIOS in the launcher.
The executable loads its optional bezel DLL automatically; a CMD launcher is unnecessary.
F10 toggles the bezel, F11 opens/closes the settings menu, and Escape exits.
Both the D-pad and left analog stick provide movement.

IPS, IPS32, BPS and UPS fan patches can be applied to the Japanese cartridge dump.
English v1.3 and its recognized HQ Audio variant can use generated native code.
Other patch outputs use the compatibility interpreter and may run more slowly.
The Android source is maintained in [Mother3Recomp-Android](https://github.com/josiahg2502-glitch/Mother3Recomp-Android).

## Building

Run `python tools/setup_deps.py` to fetch the revisions in `dependencies.lock.json`
and apply the reviewed source patch bundles. Existing checkouts are never reset.
Use CMake, Ninja, a C++20 compiler, SDL2, and OpenGL development files.
Generated cart code is private and excluded from Git. `tools/regen.sh` regenerates
it from your ROM and BIOS. Without it, the source build uses the interpreter.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Mother3Recomp
ctest --test-dir build --output-on-failure
```

On Windows, build with MinGW and `GBARECOMP_STATIC_RELEASE=OFF` so the bezel hook
uses the bundled SDL2 DLL. Build the `mother3_bezel` target alongside the game.
Convert `assets/img/bezel_master_1080p.png` to RGBA raw pixels when packaging
(`magick INPUT -depth 8 RGBA:OUTPUT`); the DLL reads `assets/img/bezel_master_1080p.raw`.

## Repository layout

- `src/`: entry point and shared game runtime hooks.
- `interposer/`: optional Windows bezel rendering; input remains in the runtime.
- `config/`, `symbols/`, `game.toml`: cartridge identity and recompilation configuration.
- `patches/`, `dependencies.lock.json`: pinned runtime source dependencies.
- `tools/`: dependency setup, build, packaging and investigation tools.
- `tests/`: menu, input and hook regression checks.
- `docs/`: player help, credits and historical release notes.

Build output, ROMs, BIOS files, signing keys, save data, and machine-specific
settings do not belong in Git. See [credits](CREDITS.md) and [license](LICENSE).
