#!/usr/bin/env bash
# Build a native MOTHER 3 .ipa for iPhone from your own ROM and BIOS, without a Mac.
#
#   tools/make_ios_app.sh <rom.gba> <gba_bios.bin> [--branch NAME] [--out DIR]
#
# 1. Generates the recompiled game + BIOS code on this computer (tools/regen.sh).
# 2. Encrypts it and uploads it to a DRAFT release on your own GitHub fork
#    (drafts are visible only to you), storing the key as a repo secret.
# 3. Runs the "iOS build (unsigned ipa)" workflow on GitHub's Mac runners.
# 4. Downloads the finished .ipa for you to sideload.
#
# The ROM, BIOS and generated code never go into git. Needs: git, gh (logged in),
# cmake, ninja, a C++20 compiler, SDL2 development files, python3, openssl.
# Your fork must have GitHub Actions enabled (Actions tab -> enable).
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
want_rom_en="306fb8874533b0fd0796208faa8bed2db4ae77fb"
want_rom_hq="4ffe71ce5c5e1e8d9be5fbd09968b60787169b21"
want_bios="300c20df6731a33952ded8c436f7f186d25d3492"
release_tag="native-corpus-private"
workflow="ios-build.yml"
state_dir="${XDG_CONFIG_HOME:-$HOME/.config}/mother3recomp-ios"

usage() { sed -n '2,15p' "$0" | sed 's/^# \{0,1\}//'; exit "${1:-0}"; }
step() { printf '\n==> %s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

rom="" bios="" branch="" out="$root/ios-ipa"
while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help) usage ;;
    --branch) branch="${2:?--branch needs a name}"; shift 2 ;;
    --out) out="${2:?--out needs a folder}"; shift 2 ;;
    -*) die "unknown option $1" ;;
    *) if [ -z "$rom" ]; then rom="$1"; elif [ -z "$bios" ]; then bios="$1"; else usage 1; fi; shift ;;
  esac
done
[ -n "$rom" ] && [ -n "$bios" ] || usage 1
[ -f "$rom" ] || die "ROM not found: $rom"
[ -f "$bios" ] || die "BIOS not found: $bios"
rom="$(cd "$(dirname "$rom")" && pwd)/$(basename "$rom")"
bios="$(cd "$(dirname "$bios")" && pwd)/$(basename "$bios")"

for tool in git gh cmake ninja python3 openssl tar; do
  command -v "$tool" >/dev/null || die "'$tool' is not installed"
done
gh auth status >/dev/null 2>&1 || die "GitHub CLI is not logged in; run: gh auth login"

sha1() { if command -v sha1sum >/dev/null; then sha1sum "$1"; else shasum -a 1 "$1"; fi | cut -d' ' -f1; }
sha256() { if command -v sha256sum >/dev/null; then sha256sum "$1"; else shasum -a 256 "$1"; fi | cut -d' ' -f1; }

step "Checking your game files"
got="$(sha1 "$rom")"
[ "$got" = "$want_rom_en" ] || [ "$got" = "$want_rom_hq" ] ||
  die "ROM SHA-1 is $got; this needs MOTHER 3 with the English fan translation v1.3 ($want_rom_en)"
got="$(sha1 "$bios")"
[ "$got" = "$want_bios" ] || die "BIOS SHA-1 is $got; expected the GBA BIOS ($want_bios)"
echo "ROM and BIOS match."

cd "$root"
# Always the 'origin' remote: gh's own repo guess prefers a fork's upstream, and
# the secret and draft release must only ever go to your fork.
origin_url="$(git remote get-url origin 2>/dev/null)" || die "this checkout has no 'origin' remote"
repo="$(printf '%s\n' "$origin_url" | sed -E 's#^(git@github\.com:|ssh://git@github\.com/|https://github\.com/)##; s#\.git$##; s#/$##')"
[[ "$repo" =~ ^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$ ]] || die "'origin' ($origin_url) is not a GitHub repository"
[ "$(gh repo view "$repo" --json viewerPermission -q .viewerPermission)" = "ADMIN" ] ||
  die "you need admin rights on $repo to store the secret; 'origin' must be your own fork"
[ -n "$branch" ] || branch="$(git rev-parse --abbrev-ref HEAD)"
git fetch -q origin "$branch" 2>/dev/null || die "branch '$branch' is not on GitHub; push it first"
[ "$(git rev-parse HEAD)" = "$(git rev-parse "origin/$branch")" ] ||
  echo "note: building origin/$branch as it is on GitHub, not your local changes"
echo "Repository: $repo (branch $branch)"

step "Preparing the recompiler (first run downloads and builds it)"
[ -d gbarecomp/.git ] || python3 tools/setup_deps.py
cmake -S . -B build-host -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build-host --target gba_recompile
tool="$root/build-host/gbarecomp_build/gba_recompile"

step "Generating native game code from your ROM"
GBA_RECOMPILE="$tool" bash tools/regen.sh "$rom" "$bios" > "$root/build-host/regen.log" 2>&1 ||
  { tail -20 "$root/build-host/regen.log"; die "code generation failed (full log: build-host/regen.log)"; }
test -f generated/dispatch_table.cpp
test -f gbarecomp/src/runtime/generated_bios/bios_recompiled.cpp

step "Encrypting the generated code"
mkdir -p "$state_dir"; chmod 700 "$state_dir"
key="$state_dir/corpus.key"
[ -s "$key" ] || { openssl rand -base64 48 > "$key"; chmod 600 "$key"; }
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
tar -czf "$work/corpus.tar.gz" generated gbarecomp/src/runtime/generated_bios
corpus_sha="$(sha256 "$work/corpus.tar.gz")"
openssl enc -aes-256-cbc -pbkdf2 -in "$work/corpus.tar.gz" -out "$work/corpus.enc" -pass "file:$key"

step "Uploading it privately to $repo"
gh secret set NATIVE_CORPUS_KEY -R "$repo" --body "$(head -n1 "$key")"
if gh release view "$release_tag" -R "$repo" >/dev/null 2>&1; then
  [ "$(gh release view "$release_tag" -R "$repo" --json isDraft -q .isDraft)" = "true" ] ||
    die "release '$release_tag' on $repo is published; delete it or turn it back into a draft"
  gh release upload "$release_tag" -R "$repo" --clobber "$work/corpus.enc"
else
  gh release create "$release_tag" -R "$repo" --draft \
    --title "private native corpus (do not publish)" \
    --notes "Encrypted code generated from a privately owned ROM, used by the iOS build. Keep this a draft." \
    "$work/corpus.enc"
fi
asset_id="$(gh api "repos/$repo/releases" \
  --jq ".[] | select(.tag_name == \"$release_tag\") | .assets[] | select(.name == \"corpus.enc\") | .id" | head -n1)"
[ -n "$asset_id" ] || die "could not find the uploaded corpus on the draft release"

step "Building the iPhone app on GitHub (about 5 minutes)"
since="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
gh workflow run "$workflow" -R "$repo" --ref "$branch" \
  -f native_asset_id="$asset_id" -f corpus_sha256="$corpus_sha"
run_id=""
for _ in $(seq 1 30); do
  run_id="$(gh run list -R "$repo" -w "$workflow" -b "$branch" -e workflow_dispatch -L 5 \
    --json databaseId,createdAt -q "[.[] | select(.createdAt >= \"$since\")][0].databaseId // empty")"
  [ -n "$run_id" ] && break
  sleep 4
done
[ -n "$run_id" ] || die "the build did not start; check the Actions tab of $repo"
echo "https://github.com/$repo/actions/runs/$run_id"
gh run watch "$run_id" -R "$repo" --exit-status --interval 30 >/dev/null ||
  { gh run view "$run_id" -R "$repo" --log-failed | grep -E "error:|FAILED|Undefined" | head -20 || true
    die "the build failed: https://github.com/$repo/actions/runs/$run_id"; }

step "Downloading the app"
mkdir -p "$out"
rm -f "$out/Mother3Recomp-unsigned.ipa"
gh run download "$run_id" -R "$repo" -n Mother3Recomp-unsigned-ipa -D "$out"
# The artifact holds code built from your ROM; on a public fork anyone signed in can fetch it.
gh api -X DELETE "repos/$repo/actions/artifacts/$(gh api "repos/$repo/actions/runs/$run_id/artifacts" \
  --jq '.artifacts[] | select(.name == "Mother3Recomp-unsigned-ipa") | .id')" >/dev/null 2>&1 ||
  echo "note: could not delete the build artifact from GitHub; remove it from the run page"

printf '\nDone: %s\n' "$out/Mother3Recomp-unsigned.ipa"
echo "Sideload it (Sideloadly, AltStore, SideStore, iloader, Impactor), then copy your"
echo "BIOS and ROM into the app's folder in the Files app (see README.md)."
