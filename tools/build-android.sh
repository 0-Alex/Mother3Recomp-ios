#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROM=""
BIOS=""
JOBS="2"
ABIS="arm64-v8a"

usage() {
  cat <<'EOF'
Usage:
  tools/build-android.sh --rom /path/to/mother3_en_v13.gba --bios /path/to/gba_bios.bin [--abis arm64-v8a,x86_64] [--jobs N]

Builds the Android APK from the same English v1.3 corpus as the desktop builds.
The ROM, BIOS and generated recompilation corpus stay local and are never
committed or packaged; players import their own files in the setup screen.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --rom) ROM="$2"; shift 2 ;;
    --bios) BIOS="$2"; shift 2 ;;
    --abis) ABIS="$2"; shift 2 ;;
    --jobs) JOBS="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage; exit 2 ;;
  esac
done

[[ -n "$ROM" && -f "$ROM" ]] || { echo "--rom is required" >&2; exit 2; }
[[ -n "$BIOS" && -f "$BIOS" ]] || { echo "--bios is required" >&2; exit 2; }

cd "$ROOT"
python3 tools/setup_deps.py

# A separate host build so the desktop build/ directory keeps its own settings.
HOST_BUILD="$ROOT/build-android-host"
cmake -S "$ROOT" -B "$HOST_BUILD" -G Ninja -DGBAGAME_RECOMP_UI=OFF
cmake --build "$HOST_BUILD" --target gba_recompile -- -j"$JOBS"

echo "Generating the MOTHER 3 and BIOS corpora..."
GBA_RECOMPILE="$HOST_BUILD/gbarecomp_build/gba_recompile" tools/regen.sh "$ROM" "$BIOS"

command -v gradle >/dev/null 2>&1 || {
  echo "Gradle is required (8.11.x recommended)." >&2
  exit 5
}

echo "Building the Android APK..."
(
  cd "$ROOT/android"
  gradle :app:assembleRelease -PgbaAbis="$ABIS" -PgbaNativeJobs="$JOBS"
)

echo
echo "APK output:"
find "$ROOT/android/app/build/outputs/apk/release" -maxdepth 1 -name '*.apk' -print
