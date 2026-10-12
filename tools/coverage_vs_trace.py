#!/usr/bin/env python3
"""Compare generated dispatch coverage with PCs executed by the mGBA reference tracer.
usage: coverage_vs_trace.py generated/dispatch_table.cpp hits.bin [hits2.bin ...]"""
import collections
import re
import sys


def main():
    if len(sys.argv) < 3:
        print(
            "usage: coverage_vs_trace.py generated/dispatch_table.cpp hits.bin [hits2.bin ...]",
            file=sys.stderr,
        )
        sys.exit(1)

    with open(sys.argv[1], "r", encoding="utf-8") as f:
        tab = {
            (int(m.group(1), 16), int(m.group(2)))
            for m in re.finditer(r"\{0x([0-9A-F]{8})u, (\d)u, \du,", f.read())
        }

    acc = None
    for path in sys.argv[2:]:
        with open(path, "rb") as f:
            buf = bytearray(f.read())
        if acc is None:
            acc = buf
        else:
            for idx, val in enumerate(buf):
                if val:
                    acc[idx] |= val

    segs = (
        (0, 0x1000000, 0x08000000),
        (0x1000000, 0x1004000, 0x03000000),
        (0x1004000, len(acc), 0x02000000),
    )
    miss = []
    total = 0
    for a, b, base in segs:
        sub = acc[a:b]
        for i, v in enumerate(sub):
            if not v:
                continue
            pc = base + i * 2
            for bit, thumb in ((1, 1), (2, 0)):
                if v & bit:
                    total += 1
                    if (pc, thumb) not in tab:
                        miss.append((pc, thumb))

    print(f"executed (pc,mode) pairs: {total}; missing from static dispatch: {len(miss)}")
    c = collections.Counter((p >> 8) << 8 for p, _ in miss)
    for k, v in sorted(c.items())[:80]:
        print(hex(k), v)
    with open("/tmp/missing_pcs.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(f'{p:08X} {"thumb" if t else "arm"}' for p, t in sorted(miss)))


if __name__ == "__main__":
    main()
