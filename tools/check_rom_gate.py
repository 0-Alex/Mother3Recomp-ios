"""Private ROM smoke check: launcher hash overrides must not select stale native code."""
import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("exe", type=Path)
parser.add_argument("rom", type=Path)
parser.add_argument("bios", type=Path)
args = parser.parse_args()
exe, bios = args.exe.resolve(), args.bios.resolve()
with tempfile.TemporaryDirectory(prefix="mother3-gate-") as temporary:
    work = Path(temporary)
    rom = bytearray(args.rom.read_bytes())
    rom[-1] ^= 1
    changed = work / "patched.gba"
    changed.write_bytes(rom)
    result = subprocess.run([str(exe), "--no-launcher", "--rom", str(changed),
        "--rom-sha1", hashlib.sha1(rom).hexdigest(), "--bios", str(bios),
        "--save-path", str(work / "test.sav"), "--state-dir", str(work),
        "--frames", "30"], cwd=work, capture_output=True, text=True, timeout=60)
    log = result.stdout + result.stderr
    if result.returncode or "force_interp=ENABLED" not in log:
        raise SystemExit("ROM gate check failed:\n" + log)
print("Unknown patch uses interpreter even with a launcher hash override: PASS")
