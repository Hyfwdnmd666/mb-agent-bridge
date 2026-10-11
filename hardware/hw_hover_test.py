"""hw_hover_test.py - separate "the game sees mouse movement" from "the game sees clicks".

Mount & Blade highlights a main-menu item when the cursor is over it. That highlight is a
response to MOVEMENT alone, so hovering isolates movement from button state:

    highlight appears   -> movement reaches the game; only the button press is being lost
    no highlight        -> the game is not receiving pointer input at all

Three cursor positions are sampled (each menu item, then a blank area as a control) and a
screenshot is taken at each one.
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

mb.focus()
time.sleep(0.4)

# client-space y for the three items, read off a screenshot; x inside the text
samples = [
    ("blank",     300, 150),
    ("tutorial",  510, 384),
    ("newgame",   510, 434),
    ("load",      510, 484),
]

for name, cx, cy in samples:
    landed = hid.move_to(ox + cx, oy + cy)
    time.sleep(0.6)
    path = f"{PLAY}\\hover_{name}.png"
    mb.screenshot(path, scale=2)
    print(f"  hover {name:9} at client ({cx},{cy})  cursor {landed}")

hid.close()

# compare each shot against the blank control: identical bytes means no highlight appeared
base = open(f"{PLAY}\\hover_blank.png", "rb").read()
for name, _, _ in samples[1:]:
    shot = open(f"{PLAY}\\hover_{name}.png", "rb").read()
    print(f"  {name:9} differs from blank: {shot != base}  ({len(shot)} vs {len(base)} bytes)")
