#!/usr/bin/env bash
# MOTHER 3 Native Linux Recompilation Runner
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

if [ -x "./Mother3Recomp" ]; then
    ./Mother3Recomp "$@"
elif [ -x "./Mother3RecompEN" ]; then
    ./Mother3RecompEN "$@"
else
    echo "MOTHER 3 Linux executable not found. Running build script to compile natively..."
    chmod +x ./tools/build_linux.sh
    ./tools/build_linux.sh
    if [ -x "./Mother3Recomp" ]; then
        ./Mother3Recomp "$@"
    fi
fi
