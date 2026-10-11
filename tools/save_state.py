#!/usr/bin/env python3
"""
save_state.py - read structured game state straight out of a Mount & Blade 1.011 save file.

Why this exists
---------------
Everything attempted through the live process fought the same three problems: memory
scanning needs a running process, input injection is rejected by the engine's DirectInput
path, and the window has to hold focus or the game auto-pauses. The save file has none of
those constraints. It is an offline, documented, structured snapshot - and it contains
exactly the state an agent needs: every party's map position, plus player identity.

Format notes (reverse-engineered from sg00.sav and cross-checked against
zhangriyueming/mb-savedata-editor, vendored under third_party/):

    offset 0    'WRDR'            magic
    offset 4    uint32 version    0x3F3 == 1011, the game's own version number
    offset 12   uint32 len + name the player name, stored as a length-prefixed string

Party map positions are (x, y) float pairs in the campaign range +-105. Each party stores
its position more than once - three consecutive entries 8 bytes apart - which makes them
trivially distinguishable from the ~1M unrelated float pairs a raw scan turns up. Positions
that repeat 5-7 times with an 8-byte stride are party records; values like (1.0, 1.0) or
(10.0, 1.0) repeating dozens of times are scale/rotation defaults and are filtered out.

Usage:
    python tools/save_state.py                       # default save location
    python tools/save_state.py path/to/sg00.sav
    python tools/save_state.py --json                # machine-readable
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from collections import defaultdict
from pathlib import Path

DEFAULT_SAVE = Path.home() / "Documents" / "Mount&Blade Savegames" / "Native" / "sg00.sav"

MAP_LIMIT = 105.0
# defaults that repeat en masse and are not party positions
NOISE = {(1.0, 1.0), (10.0, 1.0), (5.0, 1.0), (1.0, 0.6172), (0.0, 0.0)}


def read_header(data: bytes) -> dict:
    if data[:4] != b"WRDR":
        raise ValueError("not a Mount & Blade save (missing WRDR magic)")
    version = struct.unpack_from("<I", data, 4)[0]
    # the player name follows as a length-prefixed string a little further in
    name = ""
    for off in range(8, 64):
        n = struct.unpack_from("<I", data, off)[0]
        if 1 <= n <= 32 and all(32 <= b < 127 for b in data[off + 4:off + 4 + n]):
            name = data[off + 4:off + 4 + n].decode("ascii", "replace")
            break
    return {"magic": "WRDR", "version": version, "player": name}


def find_positions(data: bytes) -> list[dict]:
    """Collect float pairs in map range and group identical values."""
    groups: dict[tuple[float, float], list[int]] = defaultdict(list)
    limit = len(data) - 8
    for off in range(0, limit, 4):
        x, y = struct.unpack_from("<ff", data, off)
        if not (-MAP_LIMIT <= x <= MAP_LIMIT and -MAP_LIMIT <= y <= MAP_LIMIT):
            continue
        if abs(x) < 0.5 or abs(y) < 0.5:
            continue
        key = (round(x, 3), round(y, 3))
        if key in NOISE:
            continue
        groups[key].append(off)

    out = []
    for (x, y), offs in groups.items():
        # a real party record stores its position 3x with an 8-byte stride
        strides = [offs[i + 1] - offs[i] for i in range(min(3, len(offs) - 1))]
        clustered = sum(1 for s in strides if s == 8)
        out.append({
            "x": x, "y": y,
            "occurrences": len(offs),
            "offsets": ["0x%X" % o for o in offs[:6]],
            "stride8_cluster": clustered,
            "likely_party": clustered >= 1 and len(offs) >= 3,
        })
    out.sort(key=lambda r: (-int(r["likely_party"]), -r["occurrences"]))
    return out


def read_composition(data: bytes, rec: int, max_stacks: int = 48) -> list[tuple[int, int]]:
    """Read a party's troop stacks as (troop_id, count) pairs.

    Layout, measured around a known party record (position at record + 0x00):

        +0x18  float  facing / rotation
        +0x1C  int    stack count
        +0x20  int troop_id  +0x24 int count   +0x28 0   +0x2C 0
        +0x30  ... next stack, 16 bytes apart

    The trailing two ints of each 16-byte slot are always zero in observed saves, which is
    what makes the (id, count, 0, 0) pattern recognisable.
    """
    if rec + 0x20 >= len(data):
        return []
    stacks = []
    off = rec + 0x20
    for _ in range(max_stacks):
        if off + 16 > len(data):
            break
        troop = struct.unpack_from("<i", data, off)[0]
        count = struct.unpack_from("<i", data, off + 4)[0]
        if not (0 < troop < 2000) or not (0 < count < 10000):
            break
        stacks.append((troop, count))
        off += 16
    return stacks


def read_party_at(data: bytes, pos_offset: int) -> dict:
    """Interpret the bytes around a position pair as a party record."""
    x, y = struct.unpack_from("<ff", data, pos_offset)
    return {
        "x": round(x, 3),
        "y": round(y, 3),
        "facing": round(struct.unpack_from("<f", data, pos_offset + 0x18)[0], 4),
        "stacks": read_composition(data, pos_offset),
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("save", nargs="?", default=str(DEFAULT_SAVE))
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--all", action="store_true", help="show non-party candidates too")
    ap.add_argument("--composition", action="store_true",
                    help="also read each party's troop stacks")
    args = ap.parse_args()

    path = Path(args.save)
    if not path.is_file():
        print(f"save not found: {path}", file=sys.stderr)
        return 1
    data = path.read_bytes()

    header = read_header(data)
    rows = find_positions(data)
    parties = [r for r in rows if r["likely_party"]]

    if args.json:
        payload = {"header": header}
        if args.composition:
            parties_out = []
            for r in parties:
                rec = int(r["offsets"][0], 16)
                p = read_party_at(data, rec)
                p["occurrences"] = r["occurrences"]
                parties_out.append(p)
            payload["parties"] = parties_out
        else:
            payload["parties"] = parties if not args.all else rows
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return 0

    print(f"save      : {path}")
    print(f"size      : {len(data):,} bytes")
    print(f"magic     : {header['magic']}   version {header['version']}"
          f"{'  (= 1.011)' if header['version'] == 1011 else ''}")
    print(f"player    : {header['player']}")
    print()

    if args.composition:
        print(f"parties with composition: {len(parties)}")
        for r in parties:
            rec = int(r["offsets"][0], 16)
            p = read_party_at(data, rec)
            total = sum(c for _, c in p["stacks"])
            print(f"  ({p['x']:9.3f}, {p['y']:9.3f})  facing {p['facing']:7.4f}  "
                  f"stacks {len(p['stacks']):2d}  men {total:4d}   "
                  + " ".join(f"{t}x{c}" for t, c in p["stacks"][:6]))
        return 0

    print(f"party-like position records: {len(parties)}")
    print(f"{'x':>10} {'y':>10} {'copies':>7}  {'stride-8':>8}  first offsets")
    for r in parties:
        print(f"{r['x']:10.3f} {r['y']:10.3f} {r['occurrences']:7d}  {r['stride8_cluster']:8d}  "
              + ", ".join(r["offsets"][:3]))
    if args.all:
        print()
        print("all candidates:")
        for r in rows:
            print(f"{r['x']:10.3f} {r['y']:10.3f} {r['occurrences']:7d}  "
                  f"{'PARTY' if r['likely_party'] else ''}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
