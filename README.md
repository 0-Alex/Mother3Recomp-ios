# MOTHER 3 Native PC Port (Recompilation) v1.0.0

A native 64-bit Windows port of **MOTHER 3** featuring the official **MOTHER 3 Fan Translation v1.3 (English)**. 

Unlike conventional emulation, this project statically recompiles the original Game Boy Advance ARM/Thumb machine code into native C++ instructions. The resulting binary executes natively on modern PC hardware with ultra-low latency, full analog movement, custom bezel framing, integer scaling, and instant save state controls.

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

## 1. Requirements

### Game & Asset Requirements
To play, you must provide your own legitimate dumps of:
1. **MOTHER 3 ROM (English Patched v1.3)**:
   - **Pre-patched English ROM**: SHA-1: `306fb8874533b0fd0796208faa8bed2db4ae77fb`
   - *OR* **Clean Japanese ROM** (`A3UJ` / SHA-1: `4f0f493e12c2a8c61b2d809af03f7abf87a85776`) **+** the official `mother3.ups` patch file from the [MOTHER 3 Fan Translation website](http://mother3.fobby.net/). The built-in launcher includes an automatic UPS patcher that verifies and applies the patch on the fly without modifying your original file.
2. **Game Boy Advance BIOS Dump**:
   - `gba_bios.bin` (16 KB retail dump / SHA-1: `300c20df6731a33952ded8c436f7f186d25d3492`).

### System Requirements
| Component | Minimum | Recommended |
| :--- | :--- | :--- |
| **Operating System** | Windows 10 (64-bit) / Windows 11 | Windows 11 (64-bit) |
| **Processor** | x86-64 Dual-Core CPU with SSE2 | x86-64 Quad-Core CPU with AVX/AVX2 |
| **Memory (RAM)** | 512 MB RAM | 2 GB RAM |
| **Graphics** | Direct3D 9 / OpenGL 3.2+ compatible GPU | Direct3D 11 / Vulkan compatible GPU |
| **Storage** | 100 MB available space | 500 MB available space |
| **Input Devices** | Keyboard | Xbox One / Series, DualSense, Switch Pro, or XInput/DirectInput gamepad |

---

## 2. First-Time Setup & Quick Start

1. Download the release package [`Mother3Recomp-v1.0.0-windows-x64.zip`](https://github.com/josiahg2502-glitch/Mother3Recomp/releases/tag/v1.0.0) and extract it anywhere on your PC.
2. Run **`Play Mother 3.cmd`** or **`Mother3Recomp.exe`**.
3. On first launch, the game boots directly into the **Launcher UI**:
   - Click **Select GBA BIOS dump** and choose your `gba_bios.bin`.
   - Click **Select game ROM (.gba)** and choose your English-patched `MOTHER 3 (English v1.3).gba`.
   - *(Optional)* If using a clean Japanese ROM, tick **ROM patch** and select `mother3.ups`.
4. Configure display and controller settings if desired, then click **Play**.

---

## 3. Controls & Hotkeys

### Gameplay Controls
| Action | Keyboard | Controller (Xbox / PC) | DualShock / DualSense |
| :--- | :--- | :--- | :--- |
| **Movement** | Arrow Keys | **Left Analog Stick** / D-Pad | **Left Analog Stick** / D-Pad |
| **A (Confirm / Talk)** | `X` | `A` | `Cross` |
| **B (Cancel / Dash)** | `Z` | `B` | `Circle` |
| **L (Quick text / Info)** | `C` | `X` / `LB` | `Square` / `L1` |
| **R (Map)** | `V` | `Y` / `RB` | `Triangle` / `R1` |
| **Start (In-Game Menu)**| `Enter` | `Start` | `Options` |
| **Select (Wake Sleep)** | `Right Shift`| `Back` / `View` | `Share` / `Touchpad` |

> [!TIP]
> **Sleep Mode Wake**: If the game enters sleep mode (screen goes dark), press **`L + R + Select`** (`C + V + Right Shift` or `LB + RB + Back`) to wake it, matching authentic GBA hardware behavior.

### System Hotkeys
| Hotkey | Action | Behavior |
| :--- | :--- | :--- |
| **`F11`** | **In-Game Overlay Menu** | Opens/closes the recomp overlay menu for settings, audio, and display adjustments without closing the game. |
| **`Esc`** | **Instant Clean Exit** | Cleanly saves battery save data (SRAM/Flash) to disk and shuts down the game process immediately. |
| **`F1`** | **Quick Save State** | Saves state to Slot 1 instantly without pausing gameplay. |
| **`F2`** | **Quick Load State** | Loads state from Slot 1 instantly. |
| **`1` (Hold)** / **`LT`** | **Rewind** | Rewinds gameplay by up to 30 seconds. |
| **`2` (Hold)** / **`RT`** | **Fast-Forward** | Accelerates game speed (4x multiplier by default). |

---

## 4. Visuals & Presentation

- **1080p Sunset & Party Bezel**: Integrates official full-HD decorative border artwork featuring Lucas, Kumatora, Duster, Boney, sunflowers, and the Nowhere Islands sunset.
- **3:2 Integer Scaling**: Maps the active GBA screen into a pixel-accurate 3:2 frame with hardware alpha blending and zero clipping of HUD elements or warning text.
- **8-Way Analog Movement**: Full 360-degree analog stick translation mapped to GBA 8-directional movement with custom deadzone calibration.
- **Windows Integration**: Includes official custom multi-resolution embedded application icon (`Mother 3.ico`).

---

## 5. Building from Source

### Prerequisites
- Windows 10/11 x64 or Linux (Ubuntu 22.04+ / Debian / Fedora)
- **CMake** 3.20+
- **Ninja** build system
- **GCC / MinGW-w64** 13+ (POSIX threads)
- **TinyCC** (bundled for dynamic recompilation runtime)
- **SDL2** 2.30+ development libraries

### Compilation Steps (Windows cross-compilation or native MinGW)
```bash
# Clone the repository
git clone https://github.com/josiahg2502-glitch/Mother3Recomp.git
cd Mother3Recomp

# Build the bezel interposer DLL
cd interposer
build_bezel.bat
cd ..

# Configure and compile with CMake
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DGBARECOMP_STATIC_RELEASE=ON
cmake --build build-win --target Mother3Recomp
```

---

## 6. Credits & Disclaimers

- **MOTHER 3** © 2006 Nintendo / SHIGESATO ITOI / Brownie Brown / HAL Laboratory, Inc.
- **English Fan Translation**: MOTHER 3 Fan Translation Team ([mother3.fobby.net](http://mother3.fobby.net/)). Please support official releases and merchandise.
- **Recompilation Technology**: Powered by `gbarecomp` and `recomp-ui` by Matthew Stanley ([mstan](https://github.com/mstan)), licensed under PolyForm Noncommercial 1.0.0.
- **Third-Party Libraries**: SDL2 (zlib), TinyCC (LGPL), Dear ImGui (MIT).

*This repository is a non-commercial, open-source educational preservation project. No commercial ROMs, BIOS files, copyrighted game audio, or Nintendo proprietary code are distributed.*
