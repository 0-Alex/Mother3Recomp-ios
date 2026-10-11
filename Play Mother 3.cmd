@echo off
cd /d "%~dp0"
set SDL_DYNAMIC_API=%~dp0mother3_bezel.dll
start "" "%~dp0Mother3Recomp.exe" --fullscreen 1
