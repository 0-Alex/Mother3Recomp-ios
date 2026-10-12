#!/usr/bin/env bash
# Stage a ROM-free, BIOS-free Windows x64 package from build-win/.
# usage: tools/package_windows.sh <version> [outdir]
set -euo pipefail
ver="$1"; root="$(cd "$(dirname "$0")/.." && pwd)"
out="${2:-$root/release-stage}"; name="Mother3Recomp-windows-x64-v$ver"; stage="$out/$name"
engine="$root/gbarecomp"; tcczip="${TCC_ZIP:-$root/../deps/tcc-win64.zip}"
rm -rf "$stage"; mkdir -p "$stage/overlay_toolchain/include" "$stage/saves"
x86_64-w64-mingw32-strip -o "$stage/Mother3Recomp.exe" "$root/build-win/Mother3Recomp.exe"
cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll \
   /usr/lib/gcc/x86_64-w64-mingw32/13-posix/libstdc++-6.dll \
   /usr/lib/gcc/x86_64-w64-mingw32/13-posix/libgcc_s_seh-1.dll "$stage/"
for d in "$stage"/*.dll; do x86_64-w64-mingw32-strip --strip-unneeded "$d"; done
cp -r "$root/build-win/assets" "$stage/assets"
cp "$root/assets/img/"*.tga "$stage/assets/img/"
tmp=$(mktemp -d); (cd "$tmp" && unzip -q "$tcczip"); cp -r "$tmp/tcc" "$stage/overlay_toolchain/tcc"; rm -rf "$tmp"
for h in src/runtime/overlay_abi.h src/armv4t/runtime_wait_inline.h src/armv4t/runtime_wait_model.h; do
  [ -f "$engine/$h" ] && cp "$engine/$h" "$stage/overlay_toolchain/include/"
done
cp "$root/overlay_toolchain/include/"*.h "$stage/overlay_toolchain/include/"
# Runtime config: same as game.toml but with player-relative save path and no dev ROM/BIOS paths.
python3 - "$root/game.toml" "$stage/game.toml" <<'PY'
import sys,re
s=open(sys.argv[1]).read()
s=re.sub(r'\n\[bios\]\npath = "[^"]*"\n','\n[bios]\n',s)
s=re.sub(r'\n\[rom\]\npath = "[^"]*"\n','\n[rom]\n',s)
open(sys.argv[2],'w').write(s)
PY
cp "$root/docs/PLAYER-README.md" "$stage/README.md"
cp "$root/docs/START HERE.txt" "$stage/START HERE.txt"
cp "$root/LICENSE" "$stage/LICENSE.txt"
mkdir -p "$stage/Licenses"; cp "$root/docs/THIRD-PARTY.md" "$stage/Licenses/THIRD-PARTY.md"
cp "$stage/overlay_toolchain/tcc/doc/"*.txt "$stage/Licenses/" 2>/dev/null || true
cp "$root/../deps/SDL2-2.30.9/LICENSE.txt" "$stage/Licenses/SDL2-LICENSE.txt"
cp "$engine/LICENSE" "$stage/Licenses/gbarecomp-LICENSE.txt"; cp "$root/recomp-ui/LICENSE" "$stage/Licenses/recomp-ui-LICENSE.txt"
(cd "$out" && rm -f "$name.zip" && zip -qr "$name.zip" "$name")
echo "$stage"; du -sh "$stage" "$out/$name.zip"
