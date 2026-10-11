#!/usr/bin/env python3
"""
verify_mailbox.py - check whether the module-script bridge is alive.

The S1 experiment installs exactly one line into Modules/<mod>/simple_triggers.txt:

    0.000000 1 2105 2 <global[480]> 1        # val_add(global[480], 1) every frame

If the engine executes hand-written triggers at all, global[480] climbs constantly. That is
the single most important precondition for the structured-state / named-action design: it
proves the game runs our text without any mouse, keyboard, or injected input.

Usage:
    python tools/verify_mailbox.py            # watch for ~6 seconds
    python tools/verify_mailbox.py 480 6000   # explicit index and duration

Expected outcomes:
    CLIMBING  -> the module bridge works; continue to S2 (state read-back)
    STATIC    -> either the trigger was not loaded (game not restarted) or the engine
                 rejected it. Check the game was fully restarted after editing the file.
"""

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from mb_bridge import MBBridge, MBBridgeError  # noqa: E402


def main() -> int:
    index = int(sys.argv[1]) if len(sys.argv) > 1 else 480
    duration_ms = int(sys.argv[2]) if len(sys.argv) > 2 else 6000

    mb = MBBridge()
    try:
        block = mb.variables_block()
    except MBBridgeError as exc:
        print(f"cannot locate the variable block: {exc}")
        print("load or start a campaign first - the array only exists inside a game")
        return 1

    addr = block + index * 4
    print(f"variable block : 0x{block:08X}")
    print(f"global[{index}]    : 0x{addr:08X}")
    print(f"watching for {duration_ms} ms ...\n")

    samples = []
    t0 = time.perf_counter()
    while (time.perf_counter() - t0) * 1000 < duration_ms:
        samples.append(mb.read_var(index))
        time.sleep(0.25)

    for i, v in enumerate(samples):
        print(f"  t={i * 0.25:5.2f}s   global[{index}] = {v}")

    distinct = len(set(samples))
    print()
    if distinct > 1:
        delta = samples[-1] - samples[0]
        rate = delta / max(1e-9, (len(samples) - 1) * 0.25)
        print(f"CLIMBING - {delta} in {duration_ms} ms  (~{rate:.0f}/s)")
        print("=> the engine IS executing the trigger we hand-wrote.")
        print("   The module-script bridge works. Continue to S2.")
        return 0
    print(f"STATIC at {samples[0]}")
    print("=> the trigger is not running. Most likely the game was not restarted")
    print("   after simple_triggers.txt was edited (the engine reads it only at startup).")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
