# MOTHER 3 Recomp — Android

This module packages the same MOTHER 3 runtime and English v1.3 corpus as the
desktop builds as an Android SDL2 application. Supported language patches are
applied in the setup screen; unknown patched ROMs use the compatibility
interpreter.

A public APK does not contain a ROM or GBA BIOS. The first-launch setup Activity
imports and SHA-1 verifies the player's own files.

For a private native-recompiled build, use `../tools/build-android.sh`, which
generates the ROM/BIOS-derived C++ locally before assembling the APK.
