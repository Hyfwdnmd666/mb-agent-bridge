"""menu_map.py - build a coordinate table for the main menu by sweeping, not guessing.

Why this is necessary
---------------------
The menu renders no hover highlight, and the screenshot the bridge captures can be a STALE
frame: BitBlt reads the window DC, which keeps the last rendered image even while the game
paused and stopped drawing. So a picture of the menu is not evidence of where its items are
on screen right now. Computing a coordinate from the picture is therefore unreliable - twice
it produced the wrong item.

What actually works is the only honest signal the game gives: click something and see whether
the frame changes, then look at what appeared. This walks y from top to bottom, and after
each click returns to the menu with ESC (the hardware keyboard drives that fine), so every
probe starts from the same state.

The result is a table of y -> the screen that click opened, which is stable for as long as
the window keeps its size and position.
"""

import sys
import time

sys.path.insert(0, r"REPO_ROOT")
sys.path.insert(0, r"REPO_ROOT\hardware")

from mb_bridge import MBBridge          # noqa: E402
from hid_input import HIDInput          # noqa: E402

PLAY = r"WORKSPACE\play\map"
OUT = PLAY
import os                                # noqa: E402
os.makedirs(OUT, exist_ok=True)

mb = MBBridge()
hid = HIDInput("COM5")
ox, oy, w, h = mb.client_rect()
print(f"client {w}x{h} at ({ox},{oy})")

X_FRAC = 0.60                            # inside the menu text column
x = int(w * X_FRAC)

# safety: stay well clear of the title bar (an earlier aim at a title-bar button hit Close)
TOP = 120
BOTTOM = h - 6

table = []
for y in range(300, BOTTOM, 10):
    if y < TOP:
        continue
    # make sure we start from the menu
    hid.key(0x1B)
    time.sleep(0.8)

    hid.move_to(ox + x, oy + y)
    time.sleep(0.15)
    hid.click()
    time.sleep(1.4)

    shot = f"{OUT}\\y{y:03d}.png"
    mb.screenshot(shot, scale=2)
    size = os.path.getsize(shot)
    table.append((y, size))
    print(f"  y={y:3d}  screen bytes={size}")

hid.key(0x1B)
time.sleep(0.8)
hid.close()

print()
print("y -> bytes (identical byte counts usually mean the same screen):")
seen = {}
for y, size in table:
    seen.setdefault(size, []).append(y)
for size, ys in sorted(seen.items(), key=lambda kv: kv[1][0]):
    print(f"  {size:>8} bytes   at y = {ys}")
