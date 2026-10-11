#!/usr/bin/env python3
"""
hid_input.py - host side of the hardware input bridge.

Talks to an Arduino Pro Micro running hardware/hid_bridge/hid_bridge.ino over a serial port
and drives it as a real USB mouse and keyboard.

Why hardware instead of an API
------------------------------
Mount & Blade 1.011 reads input through DirectInput device state. Two software channels were
tried against a live game and both failed, verified by byte-identical frames before and after:

    SendInput    synthetic events carry an injected flag; DirectInput ignores them
    PostMessage  never reaches the device at all; the engine does not read its message queue

A physical USB HID device cannot be distinguished from a human's mouse, because at the
protocol level it is one. That makes this the only input channel that works against this
class of engine - and it is engine-agnostic, so it applies to every DirectInput game.

Absolute positioning with a relative device
-------------------------------------------
The Arduino Mouse library only reports relative deltas, so there is no "move to (x, y)".
Rather than write a custom absolute HID descriptor, we close the loop: send a relative delta,
read the real cursor position with GetCursorPos, and send a correction. Two iterations
normally land within a pixel, and the result is verified rather than assumed.

Usage
-----
    from hid_input import HIDInput
    hid = HIDInput()                     # auto-detects the port
    hid.move_to(client_origin + x, y)    # absolute, verified
    hid.click()
    hid.key(0x1B)                        # ESC

    python hardware/hid_input.py --list          # show candidate ports
    python hardware/hid_input.py --selftest      # prove the round trip works
"""

from __future__ import annotations

import argparse
import ctypes
import ctypes.wintypes as wt
import sys
import time
from pathlib import Path

try:
    import serial  # pyserial
    from serial.tools import list_ports
except ImportError:  # pragma: no cover - reported clearly at use time
    serial = None
    list_ports = None

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

user32 = ctypes.WinDLL("user32", use_last_error=True)

# Windows virtual key -> USB HID keyboard usage id.
# Only the keys Mount & Blade actually needs; extend as required.
VK_TO_HID = {
    0x08: 0x2A,  # backspace
    0x09: 0x2B,  # tab
    0x0D: 0x28,  # enter
    0x1B: 0x29,  # escape
    0x20: 0x2C,  # space
    0x21: 0x4B,  # page up
    0x22: 0x4E,  # page down
    0x23: 0x4D,  # end
    0x24: 0x4A,  # home
    0x25: 0x50,  # left
    0x26: 0x52,  # up
    0x27: 0x4F,  # right
    0x28: 0x51,  # down
    0x30: 0x27, 0x31: 0x1E, 0x32: 0x1F, 0x33: 0x20, 0x34: 0x21,
    0x35: 0x22, 0x36: 0x23, 0x37: 0x24, 0x38: 0x25, 0x39: 0x26,
    0x41: 0x04, 0x42: 0x05, 0x43: 0x06, 0x44: 0x07, 0x45: 0x08,
    0x46: 0x09, 0x47: 0x0A, 0x48: 0x0B, 0x49: 0x0C, 0x4A: 0x0D,
    0x4B: 0x0E, 0x4C: 0x0F, 0x4D: 0x10, 0x4E: 0x11, 0x4F: 0x12,
    0x50: 0x13, 0x51: 0x14, 0x52: 0x15, 0x53: 0x16, 0x54: 0x17,
    0x55: 0x18, 0x56: 0x19, 0x57: 0x1A, 0x58: 0x1B, 0x59: 0x1C,
    0x5A: 0x1D,
}

BUTTONS = {"left": 1, "right": 2, "middle": 4}
DEFAULT_BAUD = 115200


class HIDError(RuntimeError):
    pass


class HIDInput:
    def __init__(self, port: str | None = None, baud: int = DEFAULT_BAUD, timeout: float = 1.0):
        if serial is None:
            raise HIDError("pyserial is required: pip install pyserial")
        self.port = port or self.autodetect()
        self.ser = serial.Serial(self.port, baud, timeout=timeout)
        time.sleep(2.0)                      # the Pro Micro resets when the port opens
        self.ser.reset_input_buffer()
        self._wait_ready()

    # ---- plumbing ---------------------------------------------------------------------

    @staticmethod
    def autodetect() -> str:
        """Prefer a port that looks like a Pro Micro; otherwise take the only one there is."""
        ports = list(list_ports.comports())
        if not ports:
            raise HIDError("no serial ports found - is the Pro Micro plugged in?")
        for p in ports:
            blob = f"{p.description or ''} {p.manufacturer or ''} {p.hwid or ''}".lower()
            if any(k in blob for k in ("pro micro", "leonardo", "atmega32u4", "sparkfun", "arduino")):
                return p.device
        if len(ports) == 1:
            return ports[0].device
        raise HIDError("several serial ports and none identified as the bridge; pass port= "
                       + ", ".join(p.device for p in ports))

    def _wait_ready(self, tries: int = 10) -> None:
        for _ in range(tries):
            line = self.ser.readline().decode("ascii", "replace").strip()
            if line.startswith("ready"):
                return
        # not fatal: the device may have booted before the port opened
        self.ser.reset_input_buffer()

    def _cmd(self, text: str) -> str:
        self.ser.write((text + "\n").encode("ascii"))
        self.ser.flush()
        reply = self.ser.readline().decode("ascii", "replace").strip()
        if reply != "ok":
            raise HIDError(f"command {text!r} -> {reply!r}")
        return reply

    def ping(self) -> bool:
        try:
            return self._cmd("p") == "ok"
        except HIDError:
            return False

    def close(self) -> None:
        try:
            self.ser.close()
        except Exception:
            pass

    # ---- mouse ------------------------------------------------------------------------

    @staticmethod
    def cursor() -> tuple[int, int]:
        pt = wt.POINT()
        user32.GetCursorPos(ctypes.byref(pt))
        return pt.x, pt.y

    def move_relative(self, dx: int, dy: int) -> None:
        self._cmd(f"m {int(dx)} {int(dy)}")

    def move_to(self, x: int, y: int, tolerance: int = 1, passes: int = 3) -> tuple[int, int]:
        """Absolute move with feedback: relative deltas plus cursor readback.

        Returns the final cursor position, which is what actually matters - the caller can
        check it instead of trusting it.
        """
        for _ in range(passes):
            cx, cy = self.cursor()
            dx, dy = int(x) - cx, int(y) - cy
            if abs(dx) <= tolerance and abs(dy) <= tolerance:
                return cx, cy
            if dx or dy:
                self.move_relative(dx, dy)
            time.sleep(0.02)
        return self.cursor()

    def down(self, button: str = "left") -> None:
        self._cmd(f"d {BUTTONS[button]}")

    def up(self, button: str = "left") -> None:
        self._cmd(f"u {BUTTONS[button]}")

    def click(self, button: str = "left") -> None:
        self._cmd(f"c {BUTTONS[button]}")

    def wheel(self, notches: int) -> None:
        """Scroll the wheel. Positive scrolls down, negative up.

        Added after character creation turned out to extend past the bottom of the window:
        without a wheel there was no way to reach the attribute and weapon-proficiency rows.
        """
        self._cmd(f"r {int(notches)}")

    # ---- keyboard ---------------------------------------------------------------------

    def key(self, vk: int) -> None:
        hid = VK_TO_HID.get(vk)
        if hid is None:
            raise HIDError(f"no HID usage mapped for VK 0x{vk:02X}")
        self._cmd(f"k {hid}")

    def key_down(self, vk: int) -> None:
        hid = VK_TO_HID.get(vk)
        if hid is None:
            raise HIDError(f"no HID usage mapped for VK 0x{vk:02X}")
        self._cmd(f"K {hid}")

    def key_up(self, vk: int) -> None:
        hid = VK_TO_HID.get(vk)
        if hid is None:
            raise HIDError(f"no HID usage mapped for VK 0x{vk:02X}")
        self._cmd(f"U {hid}")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port")
    ap.add_argument("--list", action="store_true", help="list serial ports")
    ap.add_argument("--selftest", action="store_true", help="ping, move, and report the cursor")
    args = ap.parse_args()

    if args.list:
        if list_ports is None:
            print("pyserial is required: pip install pyserial")
            return 1
        for p in list_ports.comports():
            print(f"  {p.device:10} {p.description}")
        return 0

    try:
        hid = HIDInput(args.port)
    except HIDError as exc:
        print(f"error: {exc}")
        return 1

    print(f"connected on {hid.port}")
    print(f"ping: {hid.ping()}")
    if args.selftest:
        start = hid.cursor()
        print(f"cursor before      : {start}")
        end = hid.move_to(start[0] + 40, start[1] + 25)
        print(f"after +40,+25      : {end}")
        back = hid.move_to(start[0], start[1])
        print(f"back to start      : {back}")
        ok = abs(back[0] - start[0]) <= 1 and abs(back[1] - start[1]) <= 1
        print(f"SELFTEST {'PASS' if ok else 'FAIL'}")
        return 0 if ok else 2
    hid.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
