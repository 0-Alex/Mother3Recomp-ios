# Setup and controls

## Game files

Supply your own MOTHER 3 ROM. Desktop builds request a GBA BIOS; Android can use its built-in BIOS substitute. ROMs, BIOS dumps and translation patches are not included in the downloads.

- **English:** use the English fan translation v1.3 ROM, or apply its UPS patch to a clean Japanese ROM.
- **Spanish and other patches:** v1.0.4 onward accept IPS, IPS32, BPS and UPS patches through the launcher. Start with the ROM version required by the patch.
- **HQ Audio:** built in; it applies automatically to the Japanese ROM and supported fan translations after the selected language patch. Compatible ROM mods are handled when their sound layout matches.

Unknown patch outputs use the compatibility interpreter and may run more slowly. Support varies by patch.

<details>
<summary>ROM and BIOS identification</summary>

SHA-1 values:

- Japanese ROM: `4f0f493e12c2a8c61b2d809af03f7abf87a85776`
- English v1.3: `306fb8874533b0fd0796208faa8bed2db4ae77fb`
- English + HQ Audio: `4ffe71ce5c5e1e8d9be5fbd09968b60787169b21`
- Retail GBA BIOS: `300c20df6731a33952ded8c436f7f186d25d3492` (16 KiB)

</details>

## Launch

- **Windows:** extract the ZIP and open `Mother3Recomp.exe`.
- **Linux / Steam Deck:** extract the archive and open the included launcher. Ubuntu/Debian users need `libsdl2-2.0-0` and `libgl1`. Steam Deck users can add the launcher as a Non-Steam Game.
- **Android:** install the APK, open the app and select your game files. Preserve app data when updating to keep saves.

Keep the executable with its bundled libraries and assets. Back up saves before changing builds or patches.

## Launcher

The first launch asks for your BIOS and ROM. After that, MOTHER 3's own launcher opens: the party walks around
outside Tazmily while you pick **Play**, **Settings**, **Controls** or **Mods**. Everything it shows is drawn from
your ROM; no game art ships with the download. Run with `--classic-launcher` to reopen the setup launcher.

- **Settings:** window flavor (the game's Plain to Melon), display mode, window size, scaling, screen colors,
  volume, and **Button prompts**.
- **Controls:** keyboard and controller columns. Pick a cell and press the new key or button (Escape cancels).
  On controllers the left stick always moves; the D-pad is the rebindable second input.
- **Button prompts:** the game's own text ("press the A Button") shows the button you actually use: Xbox,
  PlayStation or Nintendo labels, or your keyboard key, switching when you change controller. Set a fixed style
  in Settings if you prefer.
- **Mods:** put `.ips`, `.bps` or `.ups` patches in the `mods` folder (the page has an **Open mods folder**
  button) and tick them. Mods that change game code are marked *slower*: they run in compatibility mode.
- **Bezel (Windows):** built from your ROM on first launch in your window flavor; F10 hides it.

## Controls

| Action | Keyboard |
| --- | --- |
| Move | Arrow keys |
| Confirm / cancel | X / Z |
| L / R | C / V |
| Start / Select | Enter / Right Shift |
| Toggle bezel (Windows) | F10 |
| Runtime menu (desktop) | F11 |
| Exit (desktop) | Escape |

Controllers move with the left stick (the D-pad works too). Rebind keys and buttons on the launcher's Controls page.

## Known issues

- [Android device-specific performance reports](https://github.com/josiahg2502-glitch/Mother3Recomp/issues/9).
- [Desktop F11 crash reports](https://github.com/josiahg2502-glitch/Mother3Recomp/issues/8).

When reporting a problem, include the app version, device/OS, patch version, steps and any error text. Candidate fixes are under review; device performance still needs confirmation.

[Downloads](https://github.com/josiahg2502-glitch/Mother3Recomp#download) · [Credits](https://github.com/josiahg2502-glitch/Mother3Recomp/blob/master/CREDITS.md)
