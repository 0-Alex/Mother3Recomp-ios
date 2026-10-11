#!/usr/bin/env bash
# Regenerate ROM-derived code (Linux/WSL dev helper). Usage: tools/regen.sh [rom] [bios]
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
rom="${1:-$root/roms/mother3_en_v13.gba}"
bios="${2:-$root/gbarecomp/bios/gba_bios.bin}"
tool="$root/gbarecomp/build/gba_recompile"
want=306fb8874533b0fd0796208faa8bed2db4ae77fb
got=$(sha1sum "$rom" | cut -d' ' -f1)
[ "$got" = "$want" ] || { echo "ROM SHA-1 mismatch: $got (want $want)"; exit 1; }
if [ -f "$bios" ]; then
  "$tool" --bios "$bios" --config "$root/gbarecomp/bios/gba_bios.toml" --out "$root/gbarecomp/src/runtime/generated_bios"
else
  echo "warning: no BIOS at $bios; skipping BIOS recompilation"
fi
overlay=()
for f in "$root/symbols/oracle_entries.toml" "$root/symbols/pointer_functions.toml" "$root/symbols/leaf_functions.toml"; do [ -f "$f" ] && overlay+=(--config "$f"); done
"$tool" --rom "$rom" --config "$root/game.toml" "${overlay[@]}" --out "$root/generated" --max-functions 65536
