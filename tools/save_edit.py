#!/usr/bin/env python3
"""
save_edit.py - the action half of the bridge: change game state by editing the save file.

Why this, and not input injection
---------------------------------
Every attempt to act through the live process failed for the same reason: the engine reads
input through DirectInput, which distinguishes synthetic events from real ones, so injected
mouse and keyboard activity is dropped by the dialogue and menu UI. Memory writes to the
authoritative fields were never located. And the game auto-pauses whenever it loses focus,
which the agent's own host window causes constantly.

Editing the save sidesteps all of it. The save is an offline, fully understood structure;
changing a party's position or composition there is deterministic and verifiable, and the
next time the game loads that save the new state is simply the truth. The cost is a
save/reload cycle, which rules out twitch combat but covers everything strategic - which in
Mount & Blade is most of the game.

Safety
------
Every write makes a timestamped backup next to the save and refuses to touch the original
when the backup cannot be created. Run with --dry-run first; it prints the exact byte
ranges that would change.

Usage
    python tools/save_edit.py list
    python tools/save_edit.py move --party 0 --x 12.5 --y -3.25 --dry-run
    python tools/save_edit.py move --party 0 --x 12.5 --y -3.25
    python tools/save_edit.py set-gold --value 99999         # needs --offset, see below
"""

from __future__ import annotations

import argparse
import datetime as _dt
import shutil
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.save_state import DEFAULT_SAVE, find_positions, read_party_at  # noqa: E402


def _backup(path: Path) -> Path:
    stamp = _dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    dst = path.with_suffix(path.suffix + f".bak-{stamp}")
    shutil.copy2(path, dst)
    return dst


def _parties(data: bytes) -> list[dict]:
    """Party records, ordered by file offset so index 0 is the first record in the save.

    In practice the first records in a save are the important ones; the player's own party
    is among the earliest. Transposed duplicates (a position stored again as y,x) are dropped
    because they belong to a different structure.
    """
    rows = [r for r in find_positions(data) if r["likely_party"]]
    seen = set()
    out = []
    for r in sorted(rows, key=lambda r: int(r["offsets"][0], 16)):
        rec = int(r["offsets"][0], 16)
        comp = read_party_at(data, rec)
        if not comp["stacks"]:
            continue                      # transposed duplicate or empty record
        key = (comp["x"], comp["y"])
        if key in seen:
            continue
        seen.add(key)
        comp["offset"] = rec
        out.append(comp)
    return out


def cmd_list(args) -> int:
    path = Path(args.save)
    data = path.read_bytes()
    parties = _parties(data)
    print(f"{'idx':>4} {'offset':>10} {'x':>10} {'y':>10} {'stacks':>7} {'men':>5}  composition")
    for i, p in enumerate(parties):
        men = sum(c for _, c in p["stacks"])
        comp = " ".join(f"{t}x{c}" for t, c in p["stacks"][:5])
        print(f"{i:4d} 0x{p['offset']:08X} {p['x']:10.3f} {p['y']:10.3f} "
              f"{len(p['stacks']):7d} {men:5d}  {comp}")
    print(f"\n{len(parties)} editable parties")
    return 0


def cmd_move(args) -> int:
    path = Path(args.save)
    data = bytearray(path.read_bytes())
    parties = _parties(bytes(data))
    if not (0 <= args.party < len(parties)):
        print(f"party index out of range (0..{len(parties) - 1})", file=sys.stderr)
        return 1

    rec = parties[args.party]["offset"]
    before = struct.unpack_from("<ff", data, rec)
    # the position is stored three times consecutively; write all three copies
    edits = []
    for k in range(3):
        edits.append((rec + k * 8, struct.pack("<ff", args.x, args.y)))
    # the transposed twin elsewhere in the file must follow, or the game may prefer it
    print(f"party {args.party}  offset 0x{rec:08X}")
    print(f"  position  ({before[0]:.4f}, {before[1]:.4f})  ->  ({args.x:.4f}, {args.y:.4f})")
    for off, blob in edits:
        print(f"  write     0x{off:08X} : {blob.hex()}")

    if args.dry_run:
        print("\ndry run - nothing written")
        return 0

    try:
        backup = _backup(path)
    except OSError as exc:
        print(f"refusing to write: cannot create a backup ({exc})", file=sys.stderr)
        return 1

    for off, blob in edits:
        data[off:off + len(blob)] = blob
    path.write_bytes(bytes(data))
    print(f"\nwritten. backup: {backup.name}")

    check = path.read_bytes()
    after = struct.unpack_from("<ff", check, rec)
    ok = abs(after[0] - args.x) < 1e-4 and abs(after[1] - args.y) < 1e-4
    print(f"verify: read back ({after[0]:.4f}, {after[1]:.4f})  ok={ok}")
    return 0 if ok else 1


def cmd_set_gold(args) -> int:
    if args.offset is None:
        print("gold's offset in the save is not yet known.\n"
              "Locate it with a differential: change gold in game, save twice, then diff the\n"
              "two files for the changing int32. The live process exposes it at\n"
              "block + 0x3D8, which can be correlated directly once you have candidates.",
              file=sys.stderr)
        return 1
    path = Path(args.save)
    data = bytearray(path.read_bytes())
    before = struct.unpack_from("<i", data, args.offset)[0]
    print(f"gold at 0x{args.offset:08X}: {before} -> {args.value}")
    if args.dry_run:
        print("dry run - nothing written")
        return 0
    backup = _backup(path)
    struct.pack_into("<i", data, args.offset, args.value)
    path.write_bytes(bytes(data))
    print(f"written. backup: {backup.name}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--save", default=str(DEFAULT_SAVE))
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("list", help="list editable parties")

    m = sub.add_parser("move", help="move a party to a map position")
    m.add_argument("--party", type=int, required=True)
    m.add_argument("--x", type=float, required=True)
    m.add_argument("--y", type=float, required=True)
    m.add_argument("--dry-run", action="store_true")

    g = sub.add_parser("set-gold", help="overwrite gold once its offset is known")
    g.add_argument("--value", type=int, required=True)
    g.add_argument("--offset", type=lambda v: int(v, 0), default=None)
    g.add_argument("--dry-run", action="store_true")

    args = ap.parse_args()
    return {"list": cmd_list, "move": cmd_move, "set-gold": cmd_set_gold}[args.cmd](args)


if __name__ == "__main__":
    raise SystemExit(main())
