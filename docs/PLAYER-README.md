# Mother 3 Recomp (English)

A native Windows port of **MOTHER 3** with the **MOTHER 3 Fan Translation v1.3**.
The game's original ARM/Thumb code was statically recompiled to C++ and built as
a normal Windows program. It runs on [gbarecomp](https://github.com/mstan/gbarecomp)'s
GBA hardware runtime and its [recomp-ui](https://github.com/mstan/recomp-ui) launcher.

> **Experimental preview.** The opening, naming, the start of Chapter 1 and the
> field engine have been checked frame-for-frame against a reference emulator.
> Later chapters, battles and every event have **not** been played through yet.
> Back up your saves.

## What you need

- A **ROM**, either one of:
  - the English **MOTHER 3 Fan Translation v1.3** ROM (SHA-1 `306fb8874533b0fd0796208faa8bed2db4ae77fb`), or
  - a clean **MOTHER 3 (Japan)** ROM (SHA-1 `4f0f493e12c2a8c61b2d809af03f7abf87a85776`) **plus** `mother3.ups`
    from the official v1.3 translation download. Turn on **ROM patch** in the launcher and choose the
    `.ups` file. The launcher applies it to a cached copy, leaves your files untouched, and only accepts
    the result if it matches English v1.3 exactly.
- A **GBA BIOS** dump, `gba_bios.bin` (SHA-1 `300c20df6731a33952ded8c436f7f186d25d3492`).

The port always runs the English translation. The recompiled code was generated from the
English v1.3 image, so other versions or other hacks are refused.

## Features

- Native x64 code with **no emulator CPU core**: about 48,600 game functions are compiled ahead
  of time, and every routine the reference traces executed ran as compiled C++, with no interpreter.
- Launcher with ROM/BIOS checks, display, audio and control settings.
- Windowed, resizable and fullscreen display; integer or smooth scaling; an optional sharp filter;
  GBA screen colour profiles.
- Keyboard and controller support with rebinding.
- Normal in-game saves (Flash 512K, written to `saves/`) plus **9 save-state slots**.
- **Rewind** (up to 30 s) and **fast-forward**.
- A self-healing safety net: code the static pass missed is compiled on the fly
  by the bundled TinyCC and cached, instead of crashing.

## Controls (defaults)

| GBA | Keyboard | Controller (Xbox / PC) |
|---|---|---|
| D-Pad / Movement | Arrow keys | **Left Analog Stick** / D-Pad |
| A (Confirm / Talk) | X | **A** |
| B (Cancel / Run) | Z | **B** |
| L (Quick text) | C | **X** / LB |
| R (Map) | V | **Y** / RB |
| Start (Menu) | Enter | **Start** |
| Select (Wake sleep) | Right Shift | **Back** |

If the game is put into its **sleep mode** (the screen goes blank and silent), wake it with
**L + R + Select** (C + V + Right Shift or LB + RB + Back), just like on a real GBA.

Esc opens the in-game menu. **Shift+F1–F9** saves a state and **F1–F9** loads one.
Hold **1** (or **LT**) to rewind and hold **2** (or **RT**) to fast-forward.
Custom decorative bezel art and side pillars are available in `assets/img/`.

## Credits

- MOTHER 3 © Nintendo / HAL Laboratory / Shigesato Itoi / Brownie Brown.
- English translation: the MOTHER 3 Fan Translation team (mother3.fobby.net).
  Please support the series by buying official merchandise, including an official translation
  if one is ever released.
- gbarecomp and recomp-ui by Matthew Stanley (mstan), PolyForm Noncommercial 1.0.0.
- SDL2 (zlib), TinyCC (LGPL, unmodified, bundled as a separate program), Dear ImGui (MIT).

This is an unofficial, non-commercial fan project. It is not affiliated with or endorsed by
Nintendo. No ROM, BIOS, translation patch or Nintendo asset is distributed.
