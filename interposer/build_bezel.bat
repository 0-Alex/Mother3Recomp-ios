@echo off
rem Build mother3_bezel.dll using TinyCC or MinGW-w64
if exist "..\overlay_toolchain\tcc\tcc.exe" (
    "..\overlay_toolchain\tcc\tcc.exe" -shared -o mother3_bezel.dll mother3_bezel.c
    echo Built mother3_bezel.dll with TinyCC
) else (
    gcc -shared -O2 -s -o mother3_bezel.dll mother3_bezel.c
    echo Built mother3_bezel.dll with GCC
)
