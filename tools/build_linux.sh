#!/usr/bin/env bash
# Native Linux Build Script for MOTHER 3 Recomp
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

echo "=== MOTHER 3 Native Recompilation - Linux Build ==="
echo "Ensuring submodules and build dependencies are present..."

if [ ! -d "gbarecomp/src" ]; then
    echo "Cloning required gbarecomp and recomp-ui libraries..."
    git clone --depth 1 https://github.com/mstan/gbarecomp.git gbarecomp || true
    git clone --depth 1 https://github.com/mstan/recomp-ui.git recomp-ui || true
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
