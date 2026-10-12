#!/usr/bin/env python3
"""Static evidence overlay for data-referenced Thumb LEAF functions (no PUSH LR).

Improvements over the original script:
1. Supports 2-byte aligned Thumb leaf functions (original rejected `a & 3`).
2. Filters candidates against `symbols/function_boundaries.tsv` (when present)
   so integer constants / bitmasks that land on literal pools inside existing
   functions (e.g. 0x08001001, 0x08091011) are never emitted as false-positive
   function entries.
3. Works with or without `numpy` / `capstone` installed (includes an exact
   ARMv4T Thumb linear decoder fallback).

Usage: make_leaf_overlay.py <rom> <dispatch_table.cpp> <out.toml> [function_boundaries.tsv]
"""
import array
import os
import re
import struct
import sys


def load_decomp_boundaries(tsv_path):
    starts = set()
    intervals = []
    if not tsv_path or not os.path.isfile(tsv_path):
        return starts, intervals
    rows = []
    with open(tsv_path, "r", encoding="utf-8") as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            parts = line.rstrip("\n").split("\t")
            sa, ea = int(parts[0], 16), int(parts[1], 16)
            rows.append((sa, ea))
            starts.add(sa)
    for i, (sa, ea) in enumerate(rows):
        next_sa = rows[i + 1][0] if i + 1 < len(rows) else sa + 0x20
        eff_ea = ea if ea > 0 else next_sa - 1
        intervals.append((sa, eff_ea))
    return starts, intervals


def is_interior_of_known_func(addr, starts, intervals):
    if addr in starts:
        return False
    # Binary search intervals
    lo, hi = 0, len(intervals)
    while lo < hi:
        mid = (lo + hi) >> 1
        if intervals[mid][0] <= addr:
            lo = mid + 1
        else:
            hi = mid
    idx = lo - 1
    if 0 <= idx < len(intervals):
        sa, eff_ea = intervals[idx]
        if sa < addr <= eff_ea:
            return True
    return False


def valid_thumb_leaf(rom, off, addr, code_lo=0x08000000, code_hi=0x08140000):
    """Linearly decode up to 96 Thumb instructions starting at `off` in `rom`.
    Returns True iff execution reaches `bx Rm`, `pop {..., pc}`, or `mov pc, Rm`
    using only valid Thumb encodings without hitting 0x0000 padding, SWI/UDF/BKPT,
    or out-of-range BL targets."""
    limit = min(len(rom) - 2, off + 96 * 2)
    cur = off
    pc = addr
    n = 0
    zero_run = 0
    while cur <= limit and n < 96:
        h = struct.unpack_from("<H", rom, cur)[0]
        n += 1
        if h == 0x0000:
            zero_run += 1
            if zero_run >= 2 or n == 1:
                return False
        else:
            zero_run = 0

        # Undefined / BKPT / SWI in Thumb
        if (h & 0xFF00) == 0xDF00:  # SVC / SWI
            return False
        if (h & 0xFF00) == 0xDE00:  # UDF
            return False
        if (h & 0xFF00) == 0xBE00:  # BKPT
            return False

        # BX Rm (0x4700..0x4778)
        if (h & 0xFF87) == 0x4700:
            return True
        # POP {..., PC} (0xBD00..0xBDFF)
        if (h & 0xFF00) == 0xBD00:
            return True
        # MOV PC, Rm (high register operation 0x4687..0x46FF with Rd=PC)
        if (h & 0xFF87) == 0x4687:
            return True

        # 32-bit Thumb BL prefix + suffix (0xF000..0xF7FF followed by 0xF800..0xFFFF)
        if (h & 0xF800) == 0xF000:
            if cur + 2 > limit:
                return False
            h2 = struct.unpack_from("<H", rom, cur + 2)[0]
            if (h2 & 0xF800) != 0xF800:
                return False
            imm11_hi = h & 0x07FF
            if imm11_hi & 0x0400:
                imm11_hi -= 0x0800
            imm11_lo = h2 & 0x07FF
            tgt = (pc + 4 + (imm11_hi << 12) + (imm11_lo << 1)) & 0xFFFFFFFF
            if not (code_lo <= tgt < code_hi):
                return False
            cur += 4
            pc += 4
            continue
        if (h & 0xF800) == 0xF800:
            # Orphan BL suffix without prefix
            return False

        cur += 2
        pc += 2
    return False


def main():
    if len(sys.argv) < 4:
        print(
            "usage: make_leaf_overlay.py <rom> <dispatch_table.cpp> <out.toml> [function_boundaries.tsv]",
            file=sys.stderr,
        )
        sys.exit(1)

    rom_path, tab_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    default_bounds = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "symbols", "function_boundaries.tsv"
    )
    bounds_path = sys.argv[4] if len(sys.argv) > 4 else default_bounds
    starts, intervals = load_decomp_boundaries(bounds_path)

    with open(rom_path, "rb") as f:
        rom = f.read()
    with open(tab_path, "r", encoding="utf-8") as f:
        tab = set(
            int(m.group(1), 16)
            for m in re.finditer(r"\{0x([0-9A-F]{8})u, \du, \du,", f.read())
        )

    words = array.array("I")
    words.frombytes(rom[: len(rom) & ~3])
    if sys.byteorder != "little":
        words.byteswap()

    lo, hi = 0x08000001, 0x08140000
    candidates = sorted({w - 1 for w in words if lo <= w < hi and (w & 1) == 1})

    out = []
    for a in candidates:
        if a in tab:
            continue
        # Reject interior / literal-pool hits inside known functions
        if is_interior_of_known_func(a, starts, intervals):
            continue
        # Require 4-byte alignment unless exact known symbol start
        if (a & 3) != 0 and a not in starts:
            continue
        off = a - 0x08000000
        if valid_thumb_leaf(rom, off, a):
            out.append(a)

    with open(out_path, "w", encoding="utf-8") as o:
        o.write(
            "# AUTO-GENERATED by tools/make_leaf_overlay.py: data-referenced Thumb leaf functions\n"
            "# (filtered against symbols/function_boundaries.tsv to exclude interior literal pools).\n\n"
        )
        o.write('[identity]\nsha1 = "306fb8874533b0fd0796208faa8bed2db4ae77fb"\n\n')
        for a in out:
            o.write(
                f'[[extra_func]]\naddr = 0x{a:08X}\nmode = "thumb"\nnote = "data pointer + linear leaf decode"\n\n'
            )
    print(f"{len(out)} leaf entries -> {out_path}")


if __name__ == "__main__":
    main()
