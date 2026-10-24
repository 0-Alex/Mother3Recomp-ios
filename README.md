# MOTHER 3 Recomp for iPhone

A fork of [Josiah's MOTHER 3 Recomp](https://github.com/josiahg2502-glitch/Mother3Recomp) focused on making the game a more seamless experience on iPhone, with built-in touch controls.
Bring your own ROM and BIOS.

The game runs as natively recompiled ARM code, not through an emulator's interpreter, so it runs at full speed on iPhone without JIT.

## Touch controls

The default control scheme keeps the screen clear: only L and R are drawn, and everything else is an invisible gesture.

| Where | Action | Button |
| --- | --- | --- |
| Left half of the screen | Touch anywhere and drag | D-pad (8 directions) |
| Right half of the screen | Tap or hold | A |
| Right half of the screen | Swipe left or right | B |
| Right half of the screen | Swipe up | Start |
| Right half of the screen | Swipe down | Select |
| Top-left / top-right corners, beside the game | Press | L / R |

The movement stick follows your thumb: wherever you put it down becomes the center.

Prefer buttons you can see? Two other schemes are available (see [Settings](#settings)):

- `zones`: a visible floating stick on the left; the right half presses whichever of A or B is closer
- `pad`: the original fixed on-screen gamepad

## Controllers

Bluetooth controllers (Xbox, PlayStation, MFi) work alongside the touch controls, and the in-game button prompts follow the controller you use.

## Colors

GBA games were colored for the original GBA's dark screen, so they can look washed out on a bright modern display. This fork adds a **vivid** color setting to change the gamma giving the game a more saturated full look(which I prefer). The raw display as well as the other screen features from the original are still available here.

Strength is adjustable, and the original screen-color presets (`raw`, `classic`, `unlit`, `frontlit`, `backlit`) are still available.

There is also an optional **LCD grid** that draws thin lines between the game's pixels, I wanted this to look as similar as possible to the analogue pocket and I believe it looks tasteful if you're into that sort of thing.

## 120 Hz

On iPhones with a ProMotion display, 120 fps smooth motion is supported: an in-between frame is drawn before each game frame, so scrolling and movement look smoother. The game itself still runs at its normal speed.

## Setup

1. Sideload the `.ipa` (for example with AltStore, SideStore or Impactor).
2. Open the app once, then open the Files app: **On My iPhone → MOTHER 3**.
3. Copy in your files:
   - `bios/gba_bios.bin`: GBA BIOS
   - `roms/mother3_en_v13.gba`: MOTHER 3 with the English fan translation v1.3
4. Launch the game.

## Settings

Settings live in `config.ini` in the same folder (created on first launch). Edit it in the Files app, then fully close and reopen the game.

```ini
[Launcher]
m3_controls=gestures   ; gestures (default), zones, pad
screen=vivid           ; vivid, raw, classic, unlit, frontlit, backlit
m3_vivid_gamma=1.25    ; vivid strength: 1.0 (same as raw) up to 2.0
m3_lcd_grid=0          ; LCD grid strength: 0 (off) up to 1; try 0.25-0.5
m3_smooth_120=1        ; 120 fps smooth motion: 0 off, 1 on (120 Hz screens)
m3_widescreen=0        ; 1 = 16:9 widescreen in the field
```

## Building

There is no Mac required: the `.ipa` is built by GitHub Actions (`.github/workflows/ios-build.yml`).
The recompiled game code is generated from your own ROM with `tools/regen.sh` and is never committed. The workflow takes it as an encrypted private input and builds the interpreter-only version without it. It isn't exactly as seemless as download my ipa and open the rom but it's getting there.

## Credits

- [Josiah's MOTHER 3 Recomp](https://github.com/josiahg2502-glitch/Mother3Recomp), which this fork is built on
- [gbarecomp](https://github.com/agray0425/gbarecomp) and [recomp-ui](https://github.com/mstan/recomp-ui)(the only reason this is even possible)
- The MOTHER 3 fan translation team

See [CREDITS.md](CREDITS.md) for the full list.
