"""hw_click_test.py - drive the game with the real USB HID mouse.

The whole point of the hardware channel: Mount & Blade reads input through DirectInput
device state, which ignores synthetic events entirely. A physical USB mouse is not
distinguishable from a human's mouse, because at the protocol level it *is* one.

This script proves the channel end to end by clicking the main menu with it.
"""

import sys
import time

sys.path.insert(0, r"REPO_ROOT")
sys.path.insert(0, r"REPO_ROOT\hardware")

from mb_bridge import MBBridge          # noqa: E402
from hid_input import HIDInput          # noqa: E402

PLAY = r"WORKSPACE\play"

mb = MBBridge()
hid = HIDInput("COM5")
ox, oy, w, h = mb.client_rect()
print(f"game client {w}x{h} at ({ox},{oy})")

# focus is not input; the software path handles that fine
mb.focus()
time.sleep(0.5)
mb.screenshot(PLAY + r"\hw_before.png", scale=2)

# 读取进度 sits near the bottom of the menu, at client y ~484 of a 533-tall client
cx, cy = 510, 484
print(f"hardware-clicking client ({cx},{cy}) -> screen ({ox + cx},{oy + cy})")
landed = hid.move_to(ox + cx, oy + cy)
print(f"  cursor landed at {landed}  (wanted ({ox + cx},{oy + cy}))")
time.sleep(0.2)
hid.click()
time.sleep(3.0)

mb.screenshot(PLAY + r"\hw_after.png", scale=2)
hid.close()

before = open(PLAY + r"\hw_before.png", "rb").read()
after = open(PLAY + r"\hw_after.png", "rb").read()
print(f"  frame changed: {before != after}  ({len(before)} -> {len(after)} bytes)")
