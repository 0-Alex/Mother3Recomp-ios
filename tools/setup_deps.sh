#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
prepare() {
    local dir="$1" commit="$2" patch="$3"
    if [[ ! -d "$dir/.git" ]]; then
        [[ ! -e "$dir" ]] || { echo "$dir exists without Git metadata; move it aside first." >&2; exit 1; }
        git clone "https://github.com/mstan/$dir.git" "$dir"
        git -C "$dir" checkout "$commit"
    fi
    [[ "$(git -C "$dir" rev-parse HEAD)" == "$commit" ]] || {
        echo "$dir must be at $commit; preserve local edits and check out that revision." >&2; exit 1;
    }
    if ! git -C "$dir" apply --reverse --check "$ROOT/patches/$patch" 2>/dev/null; then
        git -C "$dir" apply --check "$ROOT/patches/$patch"
        git -C "$dir" apply "$ROOT/patches/$patch"
    fi
}
prepare gbarecomp 5e89943528aee7bc1c399bdbd035364a4b41dd10 gbarecomp-mother3.patch
prepare recomp-ui 43c40b909ee1caca5bb84dec6a2f4b4000582d60 recomp-ui-ups-and-translation.patch
git -C gbarecomp submodule update --init external/arm-recomp-core external/rbengine external/recomp-net
if [[ "${1:-}" == android ]]; then
    git -C gbarecomp submodule update --init platform/android/third_party/SDL
fi
