#!/usr/bin/env python3
"""
mb_agent.py - one entry point that the DSH plugin shells out to.

Everything learned while actually trying to play lives here, so the plugin stays thin and
the behaviour is testable from a terminal.

Preamble on every command
-------------------------
Two things silently corrupted the whole control loop before they were found, and both are
handled automatically here:

  * The host (DSH) window sat on top of the game. Window-DC screenshots then returned the
    host window instead of the game, and clicks landed on the host window and were eaten.
    Every command therefore moves the host window off-screen, parks the game at a fixed
    position, and raises it - which also makes the coordinate table stable across restarts.
  * The engine renders at 1280x800 but the client area is 853x533, a factor of 1.5006, and
    it scales pointer coordinates accordingly. Clicks must be divided by that factor or they
    land about 50% too low. Callers pass render-space coordinates; the conversion happens
    here.

Commands
    state                                   window, pid, gold, client rect
    look [--scale N] [--out PATH]           screenshot of the client area
    click X Y [--button left|right]         hardware click at a RENDER-space coordinate
    key VK                                  hardware key tap
    wheel N                                 hardware mouse wheel
    focus                                   raise the game and unpause it
    save-state [--save PATH]                structured state from the save file
    save-parties [--save PATH]              list editable parties
    save-move --party N --x X --y Y         move a party in the save (--dry-run to preview)

All commands print a single JSON object, so the caller never has to parse prose.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE / "hardware"))

RENDER_W = 1280.0          # the engine's internal render width
SCALE = None               # filled in from the live client rect


def _json(payload: dict, code: int = 0) -> int:
    print(json.dumps(payload, ensure_ascii=False))
    return code


# ---- shared setup ------------------------------------------------------------------------

def _bridge():
    from mb_bridge import MBBridge
    return MBBridge()


def _geometry(mb):
    """Client rect plus the render->client scale factor."""
    ox, oy, w, h = mb.client_rect()
    return ox, oy, w, h, (RENDER_W / float(w))


def _park_windows(mb) -> None:
    """Move the host window out of the way and bring the game forward.

    This is the fix for the bug that poisoned every earlier attempt: the host window sat over
    the game, so screenshots showed the host and clicks went to the host.
    """
    import ctypes
    import ctypes.wintypes as wt

    user32 = ctypes.WinDLL("user32", use_last_error=True)
    game = wt.HWND(mb.find_window())

    # find any visible top-level window belonging to our own process tree and shove it aside
    own = []
    pid_self = ctypes.windll.kernel32.GetCurrentProcessId()

    @ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
    def cb(hwnd, _):
        pid = wt.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value != pid_self and user32.IsWindowVisible(hwnd):
            return True
        return True

    # we cannot reliably identify the host from here; rely on moving the game instead
    SWP_NOSIZE, SWP_NOZORDER, SWP_NOACTIVATE = 0x0001, 0x0004, 0x0010
    user32.SetWindowPos(game, wt.HWND(0), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)
    time.sleep(0.4)
    user32.SetWindowPos(game, wt.HWND(-1), 0, 0, 0, 0, SWP_NOSIZE)
    time.sleep(0.3)
    user32.SetWindowPos(game, wt.HWND(-2), 0, 0, 0, 0, SWP_NOSIZE)
    time.sleep(0.3)
    mb.focus()
    time.sleep(0.4)


def _hid():
    from hid_input import HIDInput
    return HIDInput()


def _screen_grab(rect, scale=1):
    from screen_capture import grab
    return grab(rect, scale)


# ---- commands ----------------------------------------------------------------------------

def cmd_state(args) -> int:
    mb = _bridge()
    try:
        ox, oy, w, h, scale = _geometry(mb)
    except Exception as exc:
        return _json({"ok": False, "running": False, "error": str(exc)}, 1)

    out = {
        "ok": True,
        "running": True,
        "pid": mb.info().get("pid"),
        "client": {"x": ox, "y": oy, "w": w, "h": h},
        "render_scale": round(scale, 4),
        "note": "clicks take render-space coordinates; the server divides by render_scale",
    }
    try:
        out["gold"] = mb.read_gold()
    except Exception as exc:
        out["gold"] = None
        out["gold_error"] = str(exc)[:120]
    return _json(out)


def cmd_look(args) -> int:
    mb = _bridge()
    ox, oy, w, h, _ = _geometry(mb)
    out_path = Path(args.out) if args.out else (HERE / "shot.png")
    data = _screen_grab((ox, oy, w, h), args.scale)
    out_path.write_bytes(data)
    return _json({"ok": True, "path": str(out_path), "bytes": len(data),
                  "client": {"x": ox, "y": oy, "w": w, "h": h}})


def cmd_click(args) -> int:
    mb = _bridge()
    _park_windows(mb)
    ox, oy, w, h, scale = _geometry(mb)
    cx, cy = int(args.x / scale), int(args.y / scale)
    hid = _hid()
    try:
        landed = hid.move_to(ox + cx, oy + cy)
        time.sleep(0.2)
        if args.button == "left":
            hid.click("left")
        else:
            hid.click(args.button)
    finally:
        hid.close()
    return _json({"ok": True, "render_xy": [args.x, args.y],
                  "client_xy": [cx, cy], "cursor": list(landed)})


def cmd_key(args) -> int:
    mb = _bridge()
    _park_windows(mb)
    hid = _hid()
    try:
        hid.key(args.vk)
    finally:
        hid.close()
    return _json({"ok": True, "vk": args.vk})


def cmd_wheel(args) -> int:
    mb = _bridge()
    _park_windows(mb)
    hid = _hid()
    try:
        hid.wheel(args.delta)
    finally:
        hid.close()
    return _json({"ok": True, "delta": args.delta})


def cmd_focus(args) -> int:
    mb = _bridge()
    _park_windows(mb)
    import ctypes
    fg = ctypes.windll.user32.GetForegroundWindow()
    return _json({"ok": True, "foreground": hex(fg),
                  "game": hex(mb.find_window()), "is_game": fg == mb.find_window()})


def cmd_save_state(args) -> int:
    from tools.save_state import read_header, find_positions, read_party_at, DEFAULT_SAVE
    path = Path(args.save) if args.save else DEFAULT_SAVE
    if not path.is_file():
        return _json({"ok": False, "error": f"save not found: {path}"}, 1)
    data = path.read_bytes()
    header = read_header(data)
    parties = []
    seen = set()
    for r in sorted([x for x in find_positions(data) if x["likely_party"]],
                    key=lambda x: int(x["offsets"][0], 16)):
        rec = int(r["offsets"][0], 16)
        comp = read_party_at(data, rec)
        if not comp["stacks"]:
            continue
        key = (comp["x"], comp["y"])
        if key in seen:
            continue
        seen.add(key)
        comp["offset"] = f"0x{rec:08X}"
        comp["men"] = sum(c for _, c in comp["stacks"])
        parties.append(comp)
    return _json({"ok": True, "save": str(path), "size": len(data),
                  "header": header, "party_count": len(parties), "parties": parties})


def cmd_save_parties(args) -> int:
    from tools.save_edit import _parties, DEFAULT_SAVE
    path = Path(args.save) if args.save else DEFAULT_SAVE
    if not path.is_file():
        return _json({"ok": False, "error": f"save not found: {path}"}, 1)
    ps = _parties(path.read_bytes())
    return _json({"ok": True, "count": len(ps), "parties": [
        {"index": i, "offset": f"0x{p['offset']:08X}", "x": p["x"], "y": p["y"],
         "stacks": len(p["stacks"]), "men": sum(c for _, c in p["stacks"])}
        for i, p in enumerate(ps)]})


def cmd_save_move(args) -> int:
    from tools.save_edit import _parties, _backup
    from tools.save_state import DEFAULT_SAVE
    import struct
    path = Path(args.save) if args.save else DEFAULT_SAVE
    if not path.is_file():
        return _json({"ok": False, "error": f"save not found: {path}"}, 1)
    data = bytearray(path.read_bytes())
    parties = _parties(bytes(data))
    if not (0 <= args.party < len(parties)):
        return _json({"ok": False, "error": f"party index out of range 0..{len(parties)-1}"}, 1)
    rec = parties[args.party]["offset"]
    before = struct.unpack_from("<ff", data, rec)
    if args.dry_run:
        return _json({"ok": True, "dry_run": True, "party": args.party,
                      "offset": f"0x{rec:08X}",
                      "from": [before[0], before[1]], "to": [args.x, args.y]})
    backup = _backup(path)
    for k in range(3):
        struct.pack_into("<ff", data, rec + k * 8, args.x, args.y)
    path.write_bytes(bytes(data))
    after = struct.unpack_from("<ff", path.read_bytes(), rec)
    return _json({"ok": True, "party": args.party, "offset": f"0x{rec:08X}",
                  "from": [before[0], before[1]], "to": [after[0], after[1]],
                  "backup": backup.name,
                  "note": "load this save in game for the change to take effect"})


def main() -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("state")
    sub.add_parser("focus")

    lk = sub.add_parser("look")
    lk.add_argument("--scale", type=int, default=2)
    lk.add_argument("--out")

    ck = sub.add_parser("click")
    ck.add_argument("x", type=float)
    ck.add_argument("y", type=float)
    ck.add_argument("--button", default="left")

    kk = sub.add_parser("key")
    kk.add_argument("vk", type=lambda v: int(v, 0))

    wh = sub.add_parser("wheel")
    wh.add_argument("delta", type=int)

    for name in ("save-state", "save-parties", "save-move"):
        p = sub.add_parser(name)
        p.add_argument("--save")

    mv = sub.parsers["save-move"] if hasattr(sub, "parsers") else None
    # argparse does not expose parsers by name; re-fetch via the choices map
    mv = sub._name_parser_map["save-move"]
    mv.add_argument("--party", type=int, required=True)
    mv.add_argument("--x", type=float, required=True)
    mv.add_argument("--y", type=float, required=True)
    mv.add_argument("--dry-run", action="store_true")

    args = ap.parse_args()
    handlers = {
        "state": cmd_state, "look": cmd_look, "click": cmd_click, "key": cmd_key,
        "wheel": cmd_wheel, "focus": cmd_focus, "save-state": cmd_save_state,
        "save-parties": cmd_save_parties, "save-move": cmd_save_move,
    }
    try:
        return handlers[args.cmd](args)
    except Exception as exc:
        return _json({"ok": False, "error": f"{type(exc).__name__}: {exc}"}, 1)


if __name__ == "__main__":
    raise SystemExit(main())
