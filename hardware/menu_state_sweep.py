"""menu_state_sweep.py - state-aware sweep.

The first sweep was invalid because it never verified where it was: one probe opened the
new-game intro, ESC did not dismiss that screen, and every later probe then measured the same
stuck screen. Eleven consecutive samples came back identical and I misread them as one menu
item.

This version checks the state before every probe. The main menu has a stable frame, so its
byte size acts as a fingerprint; anything else means we are somewhere we should not be, and
the sweep sends ESC and re-checks before continuing. If it cannot get back, it stops and says
so rather than producing more meaningless rows.
"""

import hashlib
import os
import sys
import time

sys.path.insert(0, r"REPO_ROOT")
sys.path.insert(0, r"REPO_ROOT\hardware")

from mb_bridge import MBBridge          # noqa: E402
from hid_input import HIDInput          # noqa: E402

OUT = r"WORKSPACE\play\map2"
os.makedirs(OUT, exist_ok=True)

mb = MBBridge()
hid = HIDInput("COM5")
ox, oy, w, h = mb.client_rect()
print(f"client {w}x{h} at ({ox},{oy})")

x = int(w * 0.60)


def snap(tag: str) -> tuple[int, str]:
    path = f"{OUT}\\{tag}.png"
    mb.screenshot(path, scale=2)
    raw = open(path, "rb").read()
    return len(raw), hashlib.sha256(raw).hexdigest()[:12]


size, digest = snap("menu_ref")
MENU = digest
print(f"menu fingerprint: {MENU}  ({size} bytes)\n")


def at_menu() -> bool:
    return snap("_probe")[1] == MENU


rows = []
for y in range(200, h - 40, 12):
    # get back to the menu before probing, or stop trying
    for _ in range(3):
        if at_menu():
            break
        hid.key(0x1B)
        time.sleep(0.9)
    else:
        print(f"  y={y:3d}  cannot return to the menu - stopping")
        break

    hid.move_to(ox + x, oy + y)
    time.sleep(0.18)
    hid.click()
    time.sleep(1.5)

    size, digest = snap(f"y{y:03d}")
    kind = "MENU (no change)" if digest == MENU else f"screen {digest}"
    rows.append((y, digest, kind))
    print(f"  y={y:3d}  {kind}")

# return to the menu
for _ in range(3):
    if at_menu():
        break
    hid.key(0x1B)
    time.sleep(0.9)
hid.close()

print()
print("=== distinct screens, in y order ===")
for y, digest, kind in rows:
    print(f"  y={y:3d}  {kind}")
