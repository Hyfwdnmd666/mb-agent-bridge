"""screen_capture.py - capture the game client area from the SCREEN device context.

Why this exists
---------------
The bridge's screenshot() blits from the game window's own DC. That works for most of the
frame but silently misses layers the process renders elsewhere - in practice the left half of
the character sheet came back as blank parchment while the player could see attributes and a
Finish button there. Being blind to a layer you must click is not a small problem: it makes
every coordinate on it a guess.

Blitting from the screen DC (GetDC(NULL)) captures the composited desktop instead, so whatever
is actually visible - including anything drawn by the GPU or in a child surface - comes back.

Usage:
    python hardware/screen_capture.py out.png [--scale N]
"""

from __future__ import annotations

import argparse
import ctypes
import ctypes.wintypes as wt
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from mb_bridge import MBBridge, gdi32, user32  # noqa: E402


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG),
        ("biPlanes", wt.WORD), ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
        ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", wt.LONG),
        ("biYPelsPerMeter", wt.LONG), ("biClrUsed", wt.DWORD), ("biClrImportant", wt.DWORD),
    ]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", wt.DWORD * 3)]


def grab(rect, scale=1):
    x, y, w, h = rect
    screen_dc = user32.GetDC(None)                 # <-- the whole composited desktop
    mem_dc = gdi32.CreateCompatibleDC(screen_dc)
    bmp = gdi32.CreateCompatibleBitmap(screen_dc, w, h)
    gdi32.SelectObject(mem_dc, bmp)

    # SRCCOPY | CAPTUREBLT: include layered windows, which a plain SRCCOPY skips
    gdi32.BitBlt(mem_dc, 0, 0, w, h, screen_dc, x, y, 0x00CC0000 | 0x40000000)

    bi = BITMAPINFO()
    bi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bi.bmiHeader.biWidth = w
    bi.bmiHeader.biHeight = -h                     # negative = top-down rows
    bi.bmiHeader.biPlanes = 1
    bi.bmiHeader.biBitCount = 32
    bi.bmiHeader.biCompression = 0                 # BI_RGB

    buf = ctypes.create_string_buffer(w * h * 4)
    got = gdi32.GetDIBits(mem_dc, bmp, 0, h, buf, ctypes.byref(bi), 0)

    gdi32.DeleteObject(bmp)
    gdi32.DeleteDC(mem_dc)
    user32.ReleaseDC(None, screen_dc)

    if got == 0:
        raise RuntimeError("GetDIBits returned no scanlines")

    return _to_png(buf.raw, w, h, scale)


def _to_png(raw: bytes, w: int, h: int, scale: int) -> bytes:
    """Minimal BGRA -> PNG writer, with optional integer downscale."""
    try:
        import numpy as np
        arr = np.frombuffer(raw, dtype=np.uint8).reshape(h, w, 4)[:, :, :3]
        if scale > 1:
            arr = arr[::scale, ::scale]
        hh, ww = arr.shape[:2]
        rgb = arr[:, :, ::-1].tobytes()            # BGRA -> RGB
    except ImportError:
        stride = w * 4
        rows = []
        for yy in range(0, h, scale):
            row = raw[yy * stride:(yy + 1) * stride]
            px = bytearray()
            for xx in range(0, w, scale):
                b, g, r = row[xx * 4], row[xx * 4 + 1], row[xx * 4 + 2]
                px += bytes((r, g, b))
            rows.append(bytes(px))
        hh, ww, rgb = len(rows), w // scale, b"".join(rows)

    def chunk(tag, data):
        return (zlib.crc32(tag + data) & 0xFFFFFFFF).to_bytes(4, "big")

    out = bytearray(b"\x89PNG\r\n\x1a\n")

    def emit(tag, data):
        out.extend(len(data).to_bytes(4, "big"))
        out.extend(tag)
        out.extend(data)
        out.extend(chunk(tag, data))

    emit(b"IHDR", ww.to_bytes(4, "big") + hh.to_bytes(4, "big") + bytes((8, 2, 0, 0, 0)))
    stride = ww * 3
    scan = b"".join(b"\x00" + rgb[i * stride:(i + 1) * stride] for i in range(hh))
    emit(b"IDAT", zlib.compress(scan, 1))
    emit(b"IEND", b"")
    return bytes(out)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--scale", type=int, default=1)
    args = ap.parse_args()

    mb = MBBridge()
    rect = mb.client_rect()
    print(f"client rect: {rect}")
    png = grab(rect, args.scale)
    Path(args.out).write_bytes(png)
    print(f"saved {args.out}  ({len(png)} bytes, scale={args.scale})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
