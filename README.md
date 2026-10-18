# MOTHER 3 Recomp

MOTHER 3 for Windows, Linux and Android, built with gbarecomp and recomp-ui.
Bring your own ROM and game files.

## Download

| Platform | Version | Download |
| --- | --- | --- |
| Windows x64 | 1.0.6 | [Windows ZIP](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/download/v1.0.6/Mother3Recomp-v1.0.6-windows-x64.zip) |
| Linux x86_64 / Steam Deck | 1.0.6 | [Linux archive](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/download/v1.0.6/Mother3Recomp-v1.0.6-linux-x86_64.tar.gz) |
| Android ARM64 | 1.0.6 | [Android APK](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/download/v1.0.6/Mother3Recomp-v1.0.6-android.apk) |

**v1.0.6 updates all platforms.**

## Features

- MOTHER 3's own launcher: the party walks through the crossroads outside Tazmily, drawn from your ROM (no game art ships); it also handles first-time setup
- On-screen buttons in the game's text match your controller (Xbox, PlayStation or Nintendo) or keyboard
- Controls page for keyboard and controller; the left stick always moves
- Mods: drop `.ips`, `.bps` or `.ups` patches in the `mods` folder and tick them in the launcher
- 16:9 widescreen in the field (drawn from the real map), in battles and in cutscenes
- Window flavors, display, scaling and screen-color options; a Windows bezel built from your ROM
- Built-in high-quality audio mixing

## Screenshots

| MOTHER 3's own launcher | Widescreen in Tazmily |
| --- | --- |
| ![The launcher: the party walks through the crossroads outside Tazmily](docs/screenshots/launcher.png) | ![Tazmily in 16:9](docs/screenshots/widescreen-tazmily.png) |
| **Settings** | **Controls** |
| ![Launcher settings](docs/screenshots/launcher-settings.png) | ![Launcher controls](docs/screenshots/launcher-controls.png) |
| **Widescreen fields** | **Widescreen rooms** |
| ![A field in 16:9](docs/screenshots/widescreen-field.png) | ![A room in 16:9](docs/screenshots/widescreen-interior.png) |
| **Widescreen battles** | **Widescreen cutscenes** |
| ![A battle in 16:9](docs/screenshots/widescreen-battle.png) | ![The opening in 16:9](docs/screenshots/widescreen-opening.png) |

Everything in these pictures is drawn from the player's own ROM; none of it ships with the download.

## Start playing

1. Download and extract your platform's package, or install the Android APK.
2. Open the game; on first launch it asks for your BIOS and ROM (and a translation patch for the Japanese ROM).
3. Choose **Play**.

[Setup, patches and controls](docs/PLAYER-README.md) · [Release history](https://github.com/josiahg2502-glitch/Mother3Recomp/releases) · [Report a problem](https://github.com/josiahg2502-glitch/Mother3Recomp/issues)

## Project status

Released and actively maintained. Project metadata follows the [recomp.board outline](https://recomp.fyi/spec) in [`.recomp.json`](.recomp.json).

Language patches include Spanish; compatibility and speed vary by patch.
HQ Audio mixing is built in and applies automatically to the Japanese ROM and supported fan translations.
Android has separate controls for system bars, display fill and on-screen buttons. Device-specific performance reports remain under investigation.

[Building from source](docs/BUILDING.md) · [Android source](android/) · [Credits](CREDITS.md) · [License](LICENSE)
