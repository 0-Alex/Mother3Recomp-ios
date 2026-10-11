High-Quality Sound Mixing for MOTHER 3 (English Fan Translation v1.3)
========================================================================

Original Mod by Summer Dragonfly & ipatix
Based on 16-bit audio mixing routines from Camelot / Golden Sun
Romhacking.net Entry: https://www.romhacking.net/hacks/6951/
GitHub Repository:   https://github.com/MusicTheorist/MOTHER-3-HQ-Audio

About This Mod:
---------------
The Game Boy Advance hardware mixer processes audio channels at 8-bit resolution,
causing the notorious "GBA background hiss" and quantization noise.

This hack replaces the default m4a mixer with ipatix's custom high-fidelity mixer.
It mixes every individual audio channel at 16-bit resolution before downsampling,
drastically reducing quantization noise and making Shogo Sakai's iconic soundtrack
sound crisper, clearer, and richer.

How to Apply:
-------------
Option 1 (One-Click Automatic Patcher - Recommended):
  1. Make sure your "MOTHER 3 (English v1.3).gba" ROM is placed in the "roms" folder
     (or drag and drop your .gba file directly onto "apply_hq_sound_mod.bat").
  2. Double-click "apply_hq_sound_mod.bat".
  3. The patched ROM "MOTHER 3 (English v1.3) (HQ Audio).gba" will be generated!

Option 2 (Via Mother3Recomp Built-in Launcher):
  1. Launch "Play Mother 3.cmd" or "Mother3Recomp.exe".
  2. In the Launcher screen, select your English v1.3 ROM.
  3. Under "ROM Patch", browse and select "mother3_hq_sound_mixing.ups".
  4. Click "Play". The launcher automatically applies the patch on the fly!

Option 3 (External Patchers):
  You can also apply "mother3_hq_sound_mixing.ups" using standard UPS patchers
  such as Tsukuyomi or Floating IPS (Flips).

Compatibility & Hashes:
-----------------------
- Base ROM:    MOTHER 3 (English Fan Translation v1.3)
  SHA-1:       306fb8874533b0fd0796208faa8bed2db4ae77fb

- Patched ROM: MOTHER 3 (English v1.3 + HQ Sound Mixing)
  SHA-1:       4ffe71ce5c5e1e8d9be5fbd09968b60787169b21

Note:
This mod repoints internal track players and shares the Mersenne Twister RNG to
free up RAM for the 16-bit mixing buffer. It is fully supported natively by
Mother3Recomp v1.0.1.
