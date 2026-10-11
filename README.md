# MOTHER 3 Native Port (Recompilation) v1.0.2

A native multi-platform port of **MOTHER 3** featuring the official **MOTHER 3 Fan Translation v1.3 (English)** with native support for the **High-Quality 16-Bit Sound Mixing Mod**, clean fullscreen mode, and dedicated packages for **Windows**, **Linux**, and **Android**.

Unlike conventional emulation, this project statically recompiles the original Game Boy Advance ARM/Thumb machine code into native C++ instructions. The resulting binary executes natively on modern hardware with ultra-low latency, full analog movement, optional decorative bezel framing, integer scaling, and instant save state controls.

---

<p align="center">
  <img src="docs/img/screenshot_title.png" alt="MOTHER 3 Title Screen" width="850">
</p>

<p align="center">
  <img src="docs/img/screenshot_gameplay.png" alt="MOTHER 3 Gameplay with Sunset & Party Bezel" width="850">
</p>

---

> [!IMPORTANT]
> **ENGLISH-PATCHED ROM REQUIRED**:
> This software does **NOT** contain, distribute, or bundle any copyrighted Nintendo game data, ROM files, or BIOS dumps.
> On first launch, the program boots into the built-in launcher where you must provide your legally dumped **MOTHER 3 ROM** (patched with the v1.3 English translation) and your **GBA BIOS dump**.

---

## 1. What's New in v1.0.2

* **Clean Fullscreen / No Borders Mode**:
  - Want to play in full screen without the decorative border artwork? Press **`F10`** at any time during gameplay to toggle between the **1080p Sunset & Party Bezel** and **Clean Fullscreen Mode (No Borders)**!
  - You can also set `bezel = 0` under `[Launcher]` in `config.ini` to launch directly in clean fullscreen mode by default.
* **Classic Display Profile Compatibility**:
  - Addressed driver-level black screen reports when selecting the "Classic" color profile by decoupling the viewport scaling and texture blend fallbacks.
* **Separate Multi-Platform Packages & Forks**:
  - Dedicated releases are now downloadable individually:
    - **Windows (x64)**: [`Mother3Recomp-v1.0.2-windows-x64.zip`](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/tag/v1.0.2)
    - **Linux (x86_64 / Steam Deck)**: [`Mother3Recomp-v1.0.2-linux-x86_64.tar.gz`](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/tag/v1.0.2)
    - **Android (ARM64)**: [`Mother3Recomp-v1.0.2-android-arm64.apk`](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/tag/v1.0.2)
  - Dedicated repository forks:
    - Android Port Repository: [josiahg2502-glitch/Mother3Recomp-Android](https://github.com/josiahg2502-glitch/Mother3Recomp-Android)
    - Linux Port Repository: [josiahg2502-glitch/Mother3Recomp-Linux](https://github.com/josiahg2502-glitch/Mother3Recomp-Linux)

---

## 2. Requirements

### Game & Asset Requirements
To play, you must provide your own legitimate dumps of:
1. **MOTHER 3 ROM (English Patched v1.3)**:
   - **Standard English v1.3**: SHA-1: `306fb8874533b0fd0796208faa8bed2db4ae77fb`
   - **English v1.3 + High-Quality Sound Mixing**: SHA-1: `4ffe71ce5c5e1e8d9be5fbd09968b60787169b21`
   - *OR* **Clean Japanese ROM** (`A3UJ` / SHA-1: `4f0f493e12c2a8c61b2d809af03f7abf87a85776`) **+** the official `mother3.ups` patch file from the [MOTHER 3 Fan Translation website](http://mother3.fobby.net/). The built-in launcher includes an automatic UPS patcher that verifies and applies the patch on the fly without modifying your original file.
2. **Game Boy Advance BIOS Dump**:
   - `gba_bios.bin` (16 KB retail dump / SHA-1: `300c20df6731a33952ded8c436f7f186d25d3492`).

### System Requirements
| Component | Minimum | Recommended |
| :--- | :--- | :--- |
| **Operating System** | Windows 10/11 (64-bit), Linux (glibc 2.35+ / SteamOS), Android 8.0+ | Windows 11 (64-bit) / SteamOS |
| **Processor** | x86-64 Dual-Core CPU with SSE2 / ARM64 v8a | x86-64 Quad-Core CPU with AVX2 |
| **Memory (RAM)** | 512 MB RAM | 2 GB RAM |
| **Graphics** | Direct3D 11, OpenGL 3.2+, or Vulkan compatible GPU | Direct3D 11 / Vulkan |
| **Storage** | 100 MB available space | 500 MB available space |
| **Input Devices** | Keyboard / Touchscreen | Xbox, PlayStation, Switch Pro, or XInput/DirectInput gamepad |

---

## 3. First-Time Setup & Quick Start

### Windows (PC)
1. Download [`Mother3Recomp-v1.0.2-windows-x64.zip`](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/tag/v1.0.2) and extract it anywhere on your PC.
2. Run **`Play Mother 3.cmd`** or **`Mother3Recomp.exe`**.
3. In the Launcher:
   - Click **Select GBA BIOS dump** and choose your `gba_bios.bin`.
   - Click **Select game ROM (.gba)** and choose your English-patched `MOTHER 3 (English v1.3).gba`.
   - *(Optional)* If using a clean Japanese ROM, tick **ROM patch** and select `mother3.ups`.
4. Configure display and controller settings if desired, then click **Play**.

### Linux & Steam Deck
1. Download [`Mother3Recomp-v1.0.2-linux-x86_64.tar.gz`](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/tag/v1.0.2) and extract it:
   ```bash
   tar -xzf Mother3Recomp-v1.0.2-linux-x86_64.tar.gz
   cd Mother3Recomp-Linux
   ```
2. Place `gba_bios.bin` in `bios/` and your ROM in `roms/`.
3. Make the launcher executable and run:
   ```bash
   chmod +x run_mother3.sh build_linux.sh
   ./run_mother3.sh
   ```
4. *Steam Deck*: Add `run_mother3.sh` as a Non-Steam Game in Steam for full Game Mode handheld play!

### Android
1. Download [`Mother3Recomp-v1.0.2-android-arm64.apk`](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/tag/v1.0.2) and install it on your device.
2. Open the app; the `Mother3SetupActivity` will prompt you to select your `mother3_en.gba` and `gba_bios.bin` files.
3. Once verified, the game launches natively with full touch and Bluetooth gamepad support!

---

## 4. Controls & Hotkeys

### Gameplay Controls
| Action | Keyboard | Controller (Xbox / PC) | DualShock / DualSense | Switch Pro |
| :--- | :--- | :--- | :--- | :--- |
| **Movement** | Arrow Keys | **Left Analog Stick** / D-Pad | **Left Analog Stick** / D-Pad | **Left Analog Stick** / D-Pad |
| **A (Confirm / Talk)** | `X` | `A` (or `Y`) | `Cross` (or `Triangle`) | `A` (or `X` in Nintendo mode) |
| **B (Cancel / Dash)** | `Z` | `B` (or `X`) | `Circle` (or `Square`) | `B` (or `Y` in Nintendo mode) |
| **Dash (Alternate)** | `Z` | **`X`** *(Thumb Dash)* | **`Square`** | **`Y`** |
| **L (Quick text / Info)** | `C` | `LB` / `LT` | `L1` / `L2` | `L` / `ZL` |
| **R (Map)** | `V` | `RB` / `RT` | `R1` / `R2` | `R` / `ZR` |
| **Start (In-Game Menu)**| `Enter` | `Start` | `Options` | `+` (Plus) |
| **Select (Wake Sleep)** | `Right Shift`| `Back` / `View` | `Share` / `Touchpad` | `-` (Minus) |

> [!TIP]
> **Sleep Mode Wake**: If the game enters sleep mode (screen goes dark), press **`L + R + Select`** (`C + V + Right Shift` or `LB + RB + Back`) to wake it, matching authentic GBA hardware behavior.

### Controller Remapping & Layout Presets

Mother 3 Recomp includes a native zero-latency gamepad remapping engine inside `mother3_bezel.dll` configured through `keybinds.ini`.

> [!NOTE]
> **Why `Remap Controller.bat`?**: In the upstream `recomp-ui` launcher GUI, clicking a button under Settings $\rightarrow$ Controls opens a prompt that only listens for keyboard inputs, ignoring controller button presses. Our native DLL interposer bypasses this limitation completely at the SDL2 event layer.

**One-Click Presets (`Remap Controller.bat`)**:
Double-click **`Remap Controller.bat`** in the game folder to switch between layout presets with 1 click:
- **`[1] Xbox / PC Standard`** *(Default)*: Physical `A` = Confirm, `B` = Cancel/Dash, `X` = Dash (ergonomic thumb run!), `Y` = Confirm, `LT`/`LB` = L, `RT`/`RB` = R.
- **`[2] Nintendo / Switch`**: Physical `B` = Confirm, `A` = Cancel/Dash, `Y` = Dash, `X` = Confirm (matches authentic GBA / SNES physical button placement!).
- **`[3] Modern Trigger Dash`**: Holding **Right Trigger (RT)** dashes! Perfect for modern controller ergonomics.
- **`[4] Classic 1:1 Hardware`**: Direct 1:1 hardware pass-through (`A`=A, `B`=B, `X`=X, `Y`=Y).
- **`[5] Custom Setup`**: Interactively assign any physical button or analog trigger.

> [!TIP]
> **Live Hot-Reloading**: Changes to `keybinds.ini` are monitored dynamically in memory. When you select a preset or save `keybinds.ini`, your new controls take effect **immediately in real time** without having to restart the game!

### System Hotkeys
| Hotkey | Action | Behavior |
| :--- | :--- | :--- |
| **`F10`** | **Toggle Bezel / Borders** | Toggles dynamically between the custom Sunset & Party Bezel and Clean Fullscreen (No Borders) mode. |
| **`F11`** | **In-Game Overlay Menu** | Opens/closes the recomp overlay menu for settings, audio, and display adjustments without closing the game. |
| **`Esc`** | **Instant Clean Exit** | Cleanly saves battery save data (SRAM/Flash) to disk and shuts down the game process immediately. |
| **`F1`** | **Quick Save State** | Saves state to Slot 1 instantly without pausing gameplay. |
| **`F2`** | **Quick Load State** | Loads state from Slot 1 instantly. |
| **`1` (Hold)** / **`LT`** | **Rewind** | Rewinds gameplay by up to 30 seconds. |
| **`2` (Hold)** / **`RT`** | **Fast-Forward** | Accelerates game speed (4x multiplier by default). |

---

## 5. Visuals & Presentation

- **Clean Fullscreen & Border Options**: Press **`F10`** at any time to toggle borders off/on. In `config.ini`, set `bezel = 0` for borderless clean fullscreen by default or `bezel = 1` for the decorative border.
- **1080p Sunset & Party Bezel**: Integrates official full-HD decorative border artwork featuring Lucas, Kumatora, Duster, Boney, sunflowers, and the Nowhere Islands sunset.
- **3:2 Integer Scaling**: Maps the active GBA screen into a pixel-accurate 3:2 frame with hardware alpha blending and zero clipping of HUD elements or warning text.
- **8-Way Analog Movement**: Full 360-degree analog stick translation mapped to GBA 8-directional movement with custom deadzone calibration.

---

## 6. High-Quality Sound Mixing Mod

Mother3Recomp natively supports and bundles the celebrated **[High-Quality Sound Mixing Mod for Mother 3 (Fan Translations)](https://www.romhacking.net/hacks/6951/)** by Summer Dragonfly & ipatix.

- **How it works**: The original GBA hardware mixer mixes audio channels at 8-bit depth, producing the well-known "GBA hiss" and quantization noise. This hack replaces the mixer with 16-bit channel mixing before downsampling, giving Shogo Sakai's soundtrack crystal-clear audio clarity and fidelity.
- **Included in `mods/High-Quality Sound Mixing/`**:
  - **`apply_hq_sound_mod.bat`**: An automated one-click patcher that detects your English v1.3 ROM, generates the High-Quality Audio ROM, and automatically configures the game to boot it.
  - **`mother3_hq_sound_mixing.ups`**: Official UPS patch file for use with external tools like Tsukuyomi or Flips.
- **Easy Play**: Simply run `apply_hq_sound_mod.bat` and click `Play Mother 3.cmd`! *(Note: The built-in launcher's "ROM Patch" checkbox is reserved for the fan translation patch on a clean Japanese cartridge; use the automated patcher script for the audio mod).*

---

## 7. Credits & Disclaimers

A full, detailed breakdown of all contributors, authors, libraries, and tools is available in [**`CREDITS.md`**](CREDITS.md).

- **MOTHER 3**: © 2006 Nintendo / SHIGESATO ITOI / Brownie Brown / HAL Laboratory, Inc. Music by Shogo Sakai.
- **English Fan Translation**: The MOTHER 3 Fan Translation Team ([mother3.fobby.net](http://mother3.fobby.net/)) — Clyde "Tomato" Mandelin, Jeffman, Reid Young, and contributors.
- **High-Quality Sound Mixing Mod**: Summer Dragonfly (@MusicTheorist) and ipatix ([Romhacking.net #6951](https://www.romhacking.net/hacks/6951/) / [MusicTheorist/MOTHER-3-HQ-Audio](https://github.com/MusicTheorist/MOTHER-3-HQ-Audio)), with foundational mixing code by Haruki "Bon" Kodera (Camelot) and decompilation support from Kurausukun & theo3.
- **Recompilation Technology**: Powered by `gbarecomp` and `recomp-ui` by Matthew Stanley ([@mstan](https://github.com/mstan)), licensed under PolyForm Noncommercial 1.0.0.
- **Android Port Lead**: **YlPorts** ([github.com/YlPorts/Mother3Recomp-Android](https://github.com/YlPorts/Mother3Recomp-Android)).
- **Third-Party Libraries & Runtimes**:
  - **SDL2**: Sam Lantinga & the SDL Team (zlib license)
  - **Dear ImGui**: Omar Cornut (MIT license)
  - **TinyCC**: Fabrice Bellard (LGPL 2.1 license)
  - **MinGW-w64 / GCC**: The MinGW-w64 Project Team

*This repository is a non-commercial, open-source educational preservation project. No commercial ROMs, BIOS files, copyrighted game audio, or Nintendo proprietary code are distributed.*
