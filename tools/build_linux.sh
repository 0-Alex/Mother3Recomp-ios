#!/usr/bin/env bash
# Native Linux Build Script for MOTHER 3 Recomp
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

echo "=== MOTHER 3 Native Recompilation - Linux Build ==="
echo "Ensuring submodules and build dependencies are present..."

if [ ! -d "gbarecomp/src" ]; then
    echo "Cloning pinned gbarecomp..."
    git clone https://github.com/mstan/gbarecomp.git gbarecomp
    git -C gbarecomp checkout 5e89943528aee7bc1c399bdbd035364a4b41dd10
fi

if [ ! -d "recomp-ui/src" ]; then
    echo "Cloning pinned recomp-ui..."
    git clone https://github.com/mstan/recomp-ui.git recomp-ui
    git -C recomp-ui checkout 43c40b909ee1caca5bb84dec6a2f4b4000582d60
fi

# Apply Mother 3 translation, HQ audio SHA-1, and UPS patch support
echo "Checking and applying repository patches..."
if [ -f "patches/gbarecomp-mother3.patch" ]; then
    git -C gbarecomp apply --check "$DIR/patches/gbarecomp-mother3.patch" 2>/dev/null && \
    git -C gbarecomp apply "$DIR/patches/gbarecomp-mother3.patch" || true
fi
if [ -f "patches/recomp-ui-ups-and-translation.patch" ]; then
    git -C recomp-ui apply --check "$DIR/patches/recomp-ui-ups-and-translation.patch" 2>/dev/null && \
    git -C recomp-ui apply "$DIR/patches/recomp-ui-ups-and-translation.patch" || true
fi

# Initialize required gbarecomp submodules
echo "Initializing submodules..."
git -C gbarecomp submodule update --init external/arm-recomp-core external/rbengine external/recomp-net 2>/dev/null || true

# Check if recompiled C++ sources exist in generated/
if [ ! -f "generated/recompiled_000.cpp" ]; then
    echo "Notice: generated/ does not contain precompiled C++ sources."
    echo "If building fully static, populate generated/ via: ./tools/regen.sh <path-to-rom> <path-to-bios>"
fi

echo "Configuring CMake build..."
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release || cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Mother3Recomp -j$(nproc 2>/dev/null || echo 2)

if [ -f "build/Mother3Recomp" ]; then
    cp build/Mother3Recomp ./Mother3Recomp
    chmod +x ./Mother3Recomp
    echo "Build succeeded! Run ./tools/run_mother3.sh to play."
else
    echo "Build target finished. Check build directory for output."
fi
