#!/usr/bin/env python3
"""
agent_loop.py - the smallest thing that can be called "an agent playing Mount & Blade".

It shows the shape of the loop:

    observe   screenshot the game
    decide    ask a model what to do (stubbed here - plug in your own)
    act       click / press a key through the bridge
    repeat

Run it with the game loaded into a campaign:

    python examples/agent_loop.py --turns 20 --out frames/

Every frame is written to disk so you can review what the agent saw. With --record it also
saves a JSONL trace, which is what you want when a run goes wrong and you need to replay it.
"""

from __future__ import annotations

import argparse
import json
import random
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from mb_bridge import MBBridge, MBBridgeError  # noqa: E402


def decide(frame_path: Path, memory: dict, turn: int) -> dict:
    """Replace this with a real model call.

    Given the screenshot and whatever live state you can read, return one of:

        {"action": "click", "x": 300, "y": 300}
        {"action": "key",   "vk": 0x1B}
        {"action": "wait",  "seconds": 1.5}
        {"action": "stop",  "reason": "..."}

    The stub below just wanders the campaign map, which is enough to prove the plumbing.
    """
    w, h = memory.get("client_size", (853, 480))
    return {
        "action": "click",
        # stay away from the edges: the HUD lives there
        "x": random.randint(int(w * 0.15), int(w * 0.85)),
        "y": random.randint(int(h * 0.15), int(h * 0.85)),
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--turns", type=int, default=10)
    ap.add_argument("--out", default="frames")
    ap.add_argument("--record", action="store_true", help="also write a JSONL trace")
    ap.add_argument("--pause", type=float, default=1.2, help="seconds to let the game react")
    args = ap.parse_args()

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    mb = MBBridge()
    info = mb.info()
    print(f"attached to pid {info['pid']}  client {info['client_size'][0]}x{info['client_size'][1]}")

    trace = []
    try:
        for turn in range(args.turns):
            frame = out / f"frame_{turn:04d}.png"
            mb.screenshot(frame)

            state = dict(info)
            try:
                state["gold"] = mb.read_gold()
            except MBBridgeError:
                state["gold"] = None          # no campaign loaded yet

            action = decide(frame, state, turn)
            print(f"turn {turn:3d}  gold={state['gold']}  -> {action}")

            if args.record:
                # store the path relative to --out so traces are portable across machines
                trace.append({"turn": turn, "frame": frame.name,
                              "gold": state["gold"], "action": action})

            kind = action.get("action")
            if kind == "click":
                mb.click(action["x"], action["y"])
            elif kind == "key":
                mb.key(action["vk"])
            elif kind == "wait":
                time.sleep(action.get("seconds", 1.0))
                continue
            elif kind == "stop":
                print("agent asked to stop:", action.get("reason", ""))
                break

            time.sleep(args.pause)
    except KeyboardInterrupt:
        print("\ninterrupted")
    finally:
        if args.record and trace:
            path = out / "trace.jsonl"
            path.write_text("\n".join(json.dumps(t, ensure_ascii=False) for t in trace),
                            encoding="utf-8")
            print("trace ->", path)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
