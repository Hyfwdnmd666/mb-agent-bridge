#!/usr/bin/env python3
"""
mb_bridge.py - a minimal, dependency-free bridge that lets an agent play Mount & Blade 1.011.

Why this exists
---------------
The original Mount & Blade (1.011, 32-bit, no ASLR, ImageBase 0x00400000) has no modding API
reachable without rebuilding modules, and its 2008-era UI is picky about synthetic input.
This module gives an agent the three things it actually needs to play:

    observe  ->  screenshot()      capture the game's client area to a PNG
    act      ->  click()/key()     inject input that the old UI actually accepts
    know     ->  read_gold()       read live game memory (no ASLR => stable addresses)

Everything is ctypes + the standard library. No Pillow, no pywin32, no pip install.

The one thing that matters for input
------------------------------------
This game samples the mouse once per frame. A click whose press and release both land inside
a single frame is *invisible* to it - a whole down/up/down/up batch sent in one SendInput call
is silently dropped. Real gameplay clicks hold the button for ~70 ms (several frames).
That is why click() holds the button instead of tapping it. Verified against a real
human demonstration captured with mbwatch.ps1.

Usage
-----
    python mb_bridge.py info                 # window, client rect, process id
    python mb_bridge.py shot out.png         # screenshot the client area
    python mb_bridge.py click 400 300        # click at client coordinates
    python mb_bridge.py key 0x1B             # send a virtual key (ESC)
    python mb_bridge.py gold                 # read the player's gold from memory
    python mb_bridge.py var 246              # read a module global variable by index
    python mb_bridge.py watch 20             # log clicks for N seconds (learn from a human)

As a library
------------
    from mb_bridge import MBBridge
    mb = MBBridge()
    mb.screenshot("now.png")
    mb.click(400, 300)
    print(mb.read_gold())
"""

from __future__ import annotations

import argparse
import ctypes
import ctypes.wintypes as wt
import struct
import sys
import time
import zlib
from pathlib import Path

# --------------------------------------------------------------------------------------
# Win32 bindings
# --------------------------------------------------------------------------------------

user32 = ctypes.WinDLL("user32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)

PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010
MEM_COMMIT = 0x1000
PAGE_GUARD = 0x100
PAGE_NOACCESS = 0x01
READABLE = (0x02, 0x04, 0x08, 0x20, 0x40, 0x80)  # RO, RW, WRITECOPY, EXEC_R, EXEC_RW, EXEC_WC

INPUT_MOUSE = 0
INPUT_KEYBOARD = 1
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
MOUSEEVENTF_RIGHTDOWN = 0x0008
MOUSEEVENTF_RIGHTUP = 0x0010
KEYEVENTF_KEYUP = 0x0002

SRCCOPY = 0x00CC0020
DIB_RGB_COLORS = 0

GAME_WINDOW_CLASS = "MB Window"
GAME_WINDOW_TITLE = "Mount&Blade"

# A 24-byte marker inside the game's module-variable block; unique in ~1.1 GB.
# The player's gold sits exactly 8 bytes before it.
VARIABLE_BLOCK_SIGNATURE = bytes((
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80,
    0x01, 0x00, 0x00, 0x00, 0xDB, 0xB6, 0x6D, 0xDB,
    0xB6, 0x6D, 0xDB, 0x36, 0xDB, 0xB6, 0x1D, 0x00,
))
GOLD_DELTA = 8          # gold = signature_hit - 8
BLOCK_DELTA = 0x3E0     # start of the variable block = signature_hit - 0x3E0
GOLD_VAR_INDEX = 246    # gold lives at block + 246*4 (self-check for the mapping)


class RECT(ctypes.Structure):
    _fields_ = [("left", wt.LONG), ("top", wt.LONG),
                ("right", wt.LONG), ("bottom", wt.LONG)]


class POINT(ctypes.Structure):
    _fields_ = [("x", wt.LONG), ("y", wt.LONG)]


class MEMORY_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [("BaseAddress", ctypes.c_void_p),
                ("AllocationBase", ctypes.c_void_p),
                ("AllocationProtect", wt.DWORD),
                ("RegionSize", ctypes.c_size_t),
                ("State", wt.DWORD),
                ("Protect", wt.DWORD),
                ("Type", wt.DWORD)]


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [("dx", wt.LONG), ("dy", wt.LONG), ("mouseData", wt.DWORD),
                ("dwFlags", wt.DWORD), ("time", wt.DWORD),
                ("dwExtraInfo", ctypes.POINTER(wt.ULONG))]


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [("wVk", wt.WORD), ("wScan", wt.WORD), ("dwFlags", wt.DWORD),
                ("time", wt.DWORD), ("dwExtraInfo", ctypes.POINTER(wt.ULONG))]


class _INPUTunion(ctypes.Union):
    _fields_ = [("mi", MOUSEINPUT), ("ki", KEYBDINPUT)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wt.DWORD), ("u", _INPUTunion)]


user32.EnumWindows.argtypes = [ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM), wt.LPARAM]
user32.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
user32.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
user32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(RECT)]
user32.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(POINT)]
user32.SendInput.argtypes = [wt.UINT, ctypes.POINTER(INPUT), ctypes.c_int]
user32.SendInput.restype = wt.UINT
kernel32.VirtualQueryEx.argtypes = [wt.HANDLE, ctypes.c_void_p,
                                    ctypes.POINTER(MEMORY_BASIC_INFORMATION),
                                    ctypes.c_size_t]
kernel32.VirtualQueryEx.restype = ctypes.c_size_t


# --------------------------------------------------------------------------------------
# PNG writing (stdlib only, so the plugin stays installable with zero dependencies)
# --------------------------------------------------------------------------------------

def _bgra_to_rgb_rows(width: int, height: int, bgra: bytes, scale: int = 1) -> tuple[int, int, bytes]:
    """BGRA -> PNG-ready RGB scanlines (each prefixed with filter byte 0), optionally downscaled.

    Measured on 853x480: the naive per-pixel Python loop cost ~100 ms per frame, which
    dominated the whole control loop. Two fixes, both large:

      * numpy when it is importable (it ships with the DSH runtime) -> a few ms
      * zlib level 1 instead of 6 -> ~60 ms saved, and nobody reads the extra bytes
      * optional integer downscale -> less encode time AND fewer vision tokens

    Falls back to pure stdlib so the module still has zero hard dependencies.
    """
    try:
        import numpy as np
    except ImportError:
        np = None

    if np is not None:
        a = np.frombuffer(bgra, dtype=np.uint8)
        if a.size != width * height * 4:
            raise ValueError("unexpected frame size")
        img = a.reshape(height, width, 4)[:, :, [2, 1, 0]]      # BGRA -> RGB
        if scale > 1:
            h2, w2 = height // scale, width // scale
            img = img[:h2 * scale, :w2 * scale].reshape(h2, scale, w2, scale, 3).mean(
                axis=(1, 3)).astype(np.uint8)
            height, width = h2, w2
        rows = np.zeros((height, width * 3 + 1), dtype=np.uint8)   # leading filter byte
        rows[:, 1:] = img.reshape(height, width * 3)
        return width, height, rows.tobytes()

    # ---- pure-stdlib fallback (slower, no dependencies) ----
    stride = width * 4
    if scale > 1:
        out_w, out_h = max(1, width // scale), max(1, height // scale)
        small = bytearray(out_w * out_h * 3)
        for oy in range(out_h):
            for ox in range(out_w):
                r = g = b = n = 0
                for yy in range(oy * scale, min(oy * scale + scale, height)):
                    off = yy * stride + ox * scale * 4
                    for _ in range(min(scale, width - ox * scale)):
                        b += bgra[off]; g += bgra[off + 1]; r += bgra[off + 2]
                        off += 4; n += 1
                i = (oy * out_w + ox) * 3
                small[i], small[i + 1], small[i + 2] = r // n, g // n, b // n
        width, height, rgb = out_w, out_h, bytes(small)
    else:
        rgb = b"".join(bytes((bgra[i + 2], bgra[i + 1], bgra[i]))
                       for i in range(0, stride * height, 4))

    rows = bytearray()
    stride_rgb = width * 3
    for y in range(height):
        rows.append(0)
        rows.extend(rgb[y * stride_rgb:(y + 1) * stride_rgb])
    return width, height, bytes(rows)


def _write_png(path: str | Path, width: int, height: int, bgra: bytes,
               level: int = 1, scale: int = 1) -> None:
    """Encode a BGRA buffer as a PNG. Uses zlib from the standard library."""
    out_w, out_h, rows = _bgra_to_rgb_rows(width, height, bgra, scale)

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", out_w, out_h, 8, 2, 0, 0, 0)    # 8-bit truecolour
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", ihdr)
           + chunk(b"IDAT", zlib.compress(rows, level))
           + chunk(b"IEND", b""))
    Path(path).write_bytes(png)


# --------------------------------------------------------------------------------------
# The bridge
# --------------------------------------------------------------------------------------

class MBBridgeError(RuntimeError):
    pass


class MBBridge:
    """See and act in a running Mount & Blade 1.011 window."""

    def __init__(self, window_class: str = GAME_WINDOW_CLASS,
                 window_title: str = GAME_WINDOW_TITLE):
        self.window_class = window_class
        self.window_title = window_title
        self._hwnd: int | None = None
        self._pid: int | None = None
        self._handle: int | None = None
        self._block: int | None = None
        self._last_scan_hits = 0

    # ---- window plumbing -------------------------------------------------------------

    def find_window(self, refresh: bool = False) -> int:
        """Locate the game window. Raises if the game is not running."""
        if self._hwnd and not refresh:
            return self._hwnd

        found: list[int] = []

        @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
        def _cb(hwnd, _lparam):
            cls = ctypes.create_unicode_buffer(256)
            title = ctypes.create_unicode_buffer(512)
            user32.GetClassNameW(hwnd, cls, 256)
            user32.GetWindowTextW(hwnd, title, 512)
            if cls.value == self.window_class and title.value == self.window_title:
                found.append(int(hwnd))
                return False
            return True

        user32.EnumWindows(_cb, 0)
        if not found:
            raise MBBridgeError(
                f"game window not found (class={self.window_class!r}, title={self.window_title!r}) - is Mount & Blade running?")
        self._hwnd = found[0]
        pid = wt.DWORD()
        user32.GetWindowThreadProcessId(wt.HWND(self._hwnd), ctypes.byref(pid))
        self._pid = pid.value
        return self._hwnd

    def client_rect(self) -> tuple[int, int, int, int]:
        """(screen_x, screen_y, width, height) of the window's client area.

        Screenshots are of exactly this rectangle, and click() takes coordinates
        relative to it - so what you see in a screenshot is what you click.
        """
        hwnd = self.find_window()
        rc = RECT()
        if not user32.GetClientRect(wt.HWND(hwnd), ctypes.byref(rc)):
            raise MBBridgeError("GetClientRect failed")
        origin = POINT(0, 0)
        if not user32.ClientToScreen(wt.HWND(hwnd), ctypes.byref(origin)):
            raise MBBridgeError("ClientToScreen failed")
        return origin.x, origin.y, rc.right - rc.left, rc.bottom - rc.top

    def info(self) -> dict:
        x, y, w, h = self.client_rect()
        return {"hwnd": hex(self.find_window()), "pid": self._pid,
                "client_origin": (x, y), "client_size": (w, h)}

    # ---- observe ---------------------------------------------------------------------

    def screenshot(self, path: str | Path = "shot.png", scale: int = 1) -> Path:
        """Capture the game's client area to a PNG.

        scale=2 halves each dimension: the file it writes is what the vision model reads,
        so this is the single cheapest way to cut both encode time and inference tokens.
        """
        hwnd = self.find_window()
        _, _, w, h = self.client_rect()

        hdc_window = user32.GetDC(wt.HWND(hwnd))
        hdc_mem = gdi32.CreateCompatibleDC(hdc_window)
        hbm = gdi32.CreateCompatibleBitmap(hdc_window, w, h)
        gdi32.SelectObject(hdc_mem, hbm)

        if not gdi32.BitBlt(hdc_mem, 0, 0, w, h, hdc_window, 0, 0, SRCCOPY):
            gdi32.DeleteObject(hbm)
            gdi32.DeleteDC(hdc_mem)
            user32.ReleaseDC(wt.HWND(hwnd), hdc_window)
            raise MBBridgeError("BitBlt failed")

        class BITMAPINFOHEADER(ctypes.Structure):
            _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG),
                        ("biPlanes", wt.WORD), ("biBitCount", wt.WORD),
                        ("biCompression", wt.DWORD), ("biSizeImage", wt.DWORD),
                        ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG),
                        ("biClrUsed", wt.DWORD), ("biClrImportant", wt.DWORD)]

        bih = BITMAPINFOHEADER()
        bih.biSize = ctypes.sizeof(BITMAPINFOHEADER)
        bih.biWidth = w
        bih.biHeight = -h              # negative => top-down rows
        bih.biPlanes = 1
        bih.biBitCount = 32
        bih.biCompression = 0          # BI_RGB

        buf = ctypes.create_string_buffer(w * h * 4)
        got = gdi32.GetDIBits(hdc_mem, hbm, 0, h, buf, ctypes.byref(bih), DIB_RGB_COLORS)

        gdi32.DeleteObject(hbm)
        gdi32.DeleteDC(hdc_mem)
        user32.ReleaseDC(wt.HWND(hwnd), hdc_window)

        if got == 0:
            raise MBBridgeError("GetDIBits returned no scanlines")
        _write_png(path, w, h, buf.raw, scale=max(1, int(scale)))
        return Path(path)

    # ---- act -------------------------------------------------------------------------

    def move(self, x: int, y: int) -> None:
        """Move the cursor to a client-relative point."""
        ox, oy, _, _ = self.client_rect()
        user32.SetCursorPos(ox + int(x), oy + int(y))

    def focus(self) -> None:
        """Bring the game window to the foreground.

        SendInput targets the foreground window, so this is not optional: without it every
        synthetic click is delivered to whatever window the agent itself is running in.
        AttachThreadInput is needed because Windows blocks SetForegroundWindow from a process
        that does not currently own the foreground.
        """
        hwnd = self.find_window()
        user32.BringWindowToTop(wt.HWND(hwnd))
        user32.SetForegroundWindow(wt.HWND(hwnd))

        fg = user32.GetForegroundWindow()
        if fg == hwnd:
            return
        pid = wt.DWORD()
        target_thread = user32.GetWindowThreadProcessId(fg, ctypes.byref(pid))
        this_thread = kernel32.GetCurrentThreadId()
        if target_thread and target_thread != this_thread:
            user32.AttachThreadInput(this_thread, target_thread, True)
            user32.BringWindowToTop(wt.HWND(hwnd))
            user32.SetForegroundWindow(wt.HWND(hwnd))
            user32.SetFocus(wt.HWND(hwnd))
            user32.AttachThreadInput(this_thread, target_thread, False)

    def _mouse(self, flags: int) -> None:
        inp = INPUT(type=INPUT_MOUSE)
        inp.u.mi = MOUSEINPUT(0, 0, 0, flags, 0, None)
        if user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT)) != 1:
            raise MBBridgeError(f"SendInput(mouse 0x{flags:X}) failed: {ctypes.get_last_error()}")

    def click(self, x: int, y: int, hold_ms: int = 70, button: str = "left",
              settle_ms: int = 150) -> None:
        """Click at a client-relative point.

        Two timings matter, and both were found the hard way:

        * hold_ms (default 70) - the game samples the mouse once per frame, so a press
          shorter than a frame is dropped entirely.
        * settle_ms (default 150) - the cursor must be seen AT the target for several
          frames *before* the press. Moving and pressing inside one frame makes the game
          treat it as a stray click somewhere else, which is why dialogue options and some
          buttons appeared unclickable while map clicks worked fine.

        Real human input always has a large settle time: the hand moves the mouse, then
        presses. Reproducing that gap is what makes synthetic input behave.

        Focus is the other half of the problem. SendInput delivers to whichever window owns
        the foreground, so if the agent's own chat window has focus the click never reaches
        the game at all - it looks exactly like "the game ignored the click". We therefore
        raise the game window first, every time.
        """
        self.focus()
        self.move(x, y)
        time.sleep(settle_ms / 1000.0)                     # let the game see the new position
        down = MOUSEEVENTF_LEFTDOWN if button == "left" else MOUSEEVENTF_RIGHTDOWN
        up = MOUSEEVENTF_LEFTUP if button == "left" else MOUSEEVENTF_RIGHTUP
        self._mouse(down)
        time.sleep(hold_ms / 1000.0)
        self._mouse(up)
        time.sleep(0.08)

    def key(self, vk: int, hold_ms: int = 45) -> None:
        """Send a virtual key (press+release)."""
        for flags in (0, KEYEVENTF_KEYUP):
            inp = INPUT(type=INPUT_KEYBOARD)
            inp.u.ki = KEYBDINPUT(wVk=vk, wScan=0, dwFlags=flags, time=0, dwExtraInfo=None)
            if user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT)) != 1:
                raise MBBridgeError("SendInput(key) failed")
            time.sleep(hold_ms / 1000.0)

    def key_down(self, vk: int) -> None:
        inp = INPUT(type=INPUT_KEYBOARD)
        inp.u.ki = KEYBDINPUT(wVk=vk, wScan=0, dwFlags=0, time=0, dwExtraInfo=None)
        user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT))

    def key_up(self, vk: int) -> None:
        inp = INPUT(type=INPUT_KEYBOARD)
        inp.u.ki = KEYBDINPUT(wVk=vk, wScan=0, dwFlags=KEYEVENTF_KEYUP, time=0, dwExtraInfo=None)
        user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT))

    def fight(self, seconds: float = 15.0, swing_ms: int = 260,
              advance: bool = True, x: int | None = None, y: int | None = None) -> int:
        """Reflex combat loop - the 'little brain'.

        The reasoning loop cannot sword-fight: screenshot -> inference -> click is seconds
        per decision, while melee needs ~100 ms. So combat is split. This function is the
        fast layer: it holds W to close distance and swings on a fixed cadence, entirely
        inside one process, with no model in the loop. The agent's job is to decide *whether*
        to fight and *when to disengage* - not to time each swing.

        Returns the number of swings issued.
        """
        _, _, w, h = self.client_rect()
        cx = w // 2 if x is None else x
        cy = h // 2 if y is None else y

        self.move(cx, cy)
        time.sleep(0.05)
        if advance:
            self.key_down(0x57)                 # W
        swings = 0
        try:
            deadline = time.perf_counter() + seconds
            while time.perf_counter() < deadline:
                self._mouse(MOUSEEVENTF_LEFTDOWN)
                time.sleep(0.05)
                self._mouse(MOUSEEVENTF_LEFTUP)
                swings += 1
                time.sleep(max(0.0, swing_ms / 1000.0 - 0.05))
        finally:
            if advance:
                self.key_up(0x57)
        return swings

    # ---- know (memory) ---------------------------------------------------------------

    def _open(self) -> int:
        self.find_window()
        if self._handle:
            return self._handle
        h = kernel32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, self._pid)
        if not h:
            err = ctypes.get_last_error()
            raise MBBridgeError(
                f"OpenProcess(pid={self._pid}) failed with {err}. "
                "Run this at the same or higher integrity level as the game.")
        self._handle = h
        return h

    def _read(self, address: int, size: int) -> bytes | None:
        h = self._open()
        buf = ctypes.create_string_buffer(size)
        read = ctypes.c_size_t(0)
        ok = kernel32.ReadProcessMemory(wt.HANDLE(h), ctypes.c_void_p(address), buf,
                                        size, ctypes.byref(read))
        if not ok or read.value != size:
            return None
        return buf.raw

    def read_int(self, address: int) -> int | None:
        raw = self._read(address, 4)
        return None if raw is None else struct.unpack("<i", raw)[0]

    def variables_block(self) -> int:
        """Address of the module-variable block, found by scanning for its signature.

        The scan walks ~1.1 GB, so the result is cached for the life of the process.
        """
        if self._block:
            return self._block
        h = self._open()
        addr = 0x10000
        limit = 0x7FFF0000
        mbi = MEMORY_BASIC_INFORMATION()
        while addr < limit:
            if not kernel32.VirtualQueryEx(wt.HANDLE(h), ctypes.c_void_p(addr),
                                           ctypes.byref(mbi), ctypes.sizeof(mbi)):
                break
            base = int(mbi.BaseAddress or 0)
            size = int(mbi.RegionSize)
            if size <= 0:
                break
            prot = mbi.Protect & 0xFF
            readable = (mbi.State == MEM_COMMIT
                        and not (mbi.Protect & PAGE_GUARD)
                        and not (mbi.Protect & PAGE_NOACCESS)
                        and prot in READABLE)
            if readable and size >= len(VARIABLE_BLOCK_SIGNATURE):
                chunk = 4 << 20
                off = 0
                while off < size:
                    n = min(chunk, size - off)
                    data = self._read(base + off, n)
                    if data:
                        idx = data.find(VARIABLE_BLOCK_SIGNATURE)
                        if idx >= 0:
                            self._block = base + off + idx - BLOCK_DELTA
                            return self._block
                    off += n
            nxt = base + size
            if nxt <= addr:
                break
            addr = nxt
        raise MBBridgeError("variable-block signature not found - start or load a campaign first")

    def read_var(self, index: int) -> int:
        """Read a module global variable by index."""
        block = self.variables_block()
        val = self.read_int(block + index * 4)
        if val is None:
            raise MBBridgeError(f"could not read variable {index}")
        return val

    def read_gold(self) -> int:
        """Read the player's gold.

        Gold sits at block + 0x3D8, which is exactly signature_hit - 8, and also exactly
        block + 246*4 - so reading it doubles as a check that the block base is right.
        """
        block = self.variables_block()
        gold = self.read_int(block + 0x3D8)
        if gold is None:
            raise MBBridgeError("could not read gold")
        return gold


# --------------------------------------------------------------------------------------
# Learning mode: watch a human play
# --------------------------------------------------------------------------------------

def watch(seconds: float = 20.0, poll_ms: int = 12) -> list[dict]:
    """Record real clicks so an agent can learn this UI's true coordinates.

    This is how the correct interaction pattern was found: a human loaded a save while this
    logged every button press with its exact screen position.
    """
    VK_LBUTTON, VK_RBUTTON = 0x01, 0x02
    events: list[dict] = []
    t0 = time.time()
    last = {"l": bool(user32.GetAsyncKeyState(VK_LBUTTON) & 0x8000),
            "r": bool(user32.GetAsyncKeyState(VK_RBUTTON) & 0x8000)}
    print(f"watching for {seconds:.0f}s - go ahead and play; log goes to stdout")
    while time.time() - t0 < seconds:
        time.sleep(poll_ms / 1000.0)
        now = {"l": bool(user32.GetAsyncKeyState(VK_LBUTTON) & 0x8000),
               "r": bool(user32.GetAsyncKeyState(VK_RBUTTON) & 0x8000)}
        for btn, key in (("left", "l"), ("right", "r")):
            if now[key] and not last[key]:
                pt = POINT()
                user32.GetCursorPos(ctypes.byref(pt))
                fg = user32.GetForegroundWindow()
                cls = ctypes.create_unicode_buffer(256)
                user32.GetClassNameW(fg, cls, 256)
                ev = {"t": round(time.time() - t0, 2), "button": btn,
                      "screen": (pt.x, pt.y), "foreground_class": cls.value}
                events.append(ev)
                print(f"  [{ev['t']:7.2f}s] {btn.upper():5} at ({pt.x},{pt.y})  fg={cls.value!r}")
        last = now
    print(f"captured {len(events)} presses")
    return events


# --------------------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Mount & Blade 1.011 agent bridge")
    sub = p.add_subparsers(dest="cmd", required=True)

    sub.add_parser("info", help="window, client rect, pid")
    s = sub.add_parser("shot", help="screenshot the client area")
    s.add_argument("path", nargs="?", default="shot.png")
    s.add_argument("--scale", type=int, default=1, help="integer downscale, e.g. 2")
    c = sub.add_parser("click", help="click at client coordinates")
    c.add_argument("x", type=int); c.add_argument("y", type=int)
    c.add_argument("--hold-ms", type=int, default=70)
    k = sub.add_parser("key", help="send a virtual key")
    k.add_argument("vk", type=lambda v: int(v, 0))
    sub.add_parser("gold", help="read the player's gold from memory")
    v = sub.add_parser("var", help="read a module global variable")
    v.add_argument("index", type=int)
    w = sub.add_parser("watch", help="record real clicks for N seconds")
    w.add_argument("seconds", type=float, nargs="?", default=20.0)
    f = sub.add_parser("fight", help="reflex combat loop: advance and swing (the 'little brain')")
    f.add_argument("seconds", type=float, nargs="?", default=15.0)
    f.add_argument("--swing-ms", type=int, default=260, help="gap between swings")
    f.add_argument("--no-advance", action="store_true", help="swing without holding W")

    args = p.parse_args(argv)
    mb = MBBridge()
    try:
        if args.cmd == "info":
            for k2, val in mb.info().items():
                print(f"{k2:15} {val}")
        elif args.cmd == "shot":
            print("saved", mb.screenshot(args.path, scale=args.scale))
        elif args.cmd == "fight":
            n = mb.fight(args.seconds, swing_ms=args.swing_ms, advance=not args.no_advance)
            print(f"fight loop finished: {n} swings in {args.seconds:.0f}s")
        elif args.cmd == "click":
            mb.click(args.x, args.y, hold_ms=args.hold_ms)
            print(f"clicked client ({args.x},{args.y}) hold={args.hold_ms}ms")
        elif args.cmd == "key":
            mb.key(args.vk)
            print(f"sent vk=0x{args.vk:02X}")
        elif args.cmd == "gold":
            print(mb.read_gold())
        elif args.cmd == "var":
            print(mb.read_var(args.index))
        elif args.cmd == "watch":
            watch(args.seconds)
    except MBBridgeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
