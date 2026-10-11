"""menu_sweep.py - find the clickable row of a menu item by sweeping, not by guessing.

The main menu is a static image with no hover highlight, so a failed click gives no feedback.
Rather than compute a coordinate from the text baseline and hope, this walks down the y axis,
clicks once per row, and stops the moment the frame changes - which is the only reliable
signal the game offers here.

SAFETY: every click is strictly inside the client rectangle, well away from the title bar.
An earlier attempt that aimed at a title-bar button hit Close instead of Minimize and killed
the game; this script does not go near it.
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
print(f"client {w}x{h} at ({ox},{oy})")

# hard guard: never aim within 80 px of the client's top edge (that is chrome territory)
SAFE_TOP = 80

reference = f"{PLAY}\\sweep_ref.png"
mb.screenshot(reference, scale=2)
ref = open(reference, "rb").read()
print(f"reference frame {len(ref)} bytes\n")

x = int(w * 0.60)                     # inside the menu text column
found = []

for y in range(430, 531, 8):
    if y < SAFE_TOP:
        continue
    hid.move_to(ox + x, oy + y)
    time.sleep(0.25)
    hid.click()
    time.sleep(1.6)
    shot = f"{PLAY}\\sweep_y{y}.png"
    mb.screenshot(shot, scale=2)
    data = open(shot, "rb").read()
    changed = data != ref
    print(f"  y={y:4d}  {'CHANGED  <== hit' if changed else 'no change'}")
    if changed:
        found.append(y)
        break

hid.close()
print()
if found:
    print(f"HIT at client ({x},{found[0]})  ->  screen ({ox + x},{oy + found[0]})")
else:
    print("no y in the swept range triggered a change")
