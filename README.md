# mb-agent-bridge

**Teaching an AI agent to play *Mount & Blade* 1.011 (2008).**

> **Status: on hold, roughly 40% complete.** The observation channel works. The input channel
> works. The layer that would let an agent *understand* a screen does not exist yet, and the
> project stops there. See [Where this stopped](#where-this-stopped) for the honest account —
> including what I got wrong and how, which is most of the interesting content in this repo.
>
> **[中文停工说明 →](docs/status.zh.md)**

No mods. No DLL injection. No third-party packages beyond `pyserial` for the hardware link.

---

## What actually works

Everything below was verified against the running game, not assumed.

| Capability | Status | Evidence |
|---|---|---|
| Read campaign state from the **save file** | ✅ works | 41 parties with position, facing, troop stacks and men; `WRDR` magic + version self-identifies as 1011 |
| Write campaign state to the **save file** | ✅ works | moved a party, read it back and confirmed, automatic timestamped backup |
| Move the mouse with **pixel accuracy** | ✅ works | self-test: out 60,40 and back, landed within 1 px |
| Click, press keys, scroll the wheel | ✅ works | opened the Options, Credits and New Game screens by driving the game |
| Screenshot the game | ✅ works | from the composited desktop, after the occlusion bug was found |
| Read money from live memory | ✅ works | `block + 0x3D8`, cross-checked against the on-screen value |
| **Click anything in a menu or dialogue** | ❌ does not work | see below |
| **Understand what is on screen** | ❌ not attempted properly | this is the real gap |

---

## Why the input problem was hard, and what actually solved it

This is the part worth reading if you are automating any game from this era.

**The engine reads pointer state through DirectInput.** That reads *device* state, and it
ignores synthetic events. Two software channels were tried against a live game and both failed,
verified by byte-identical frames before and after:

```
SendInput      synthetic events carry an injected flag; DirectInput ignores them
PostMessage    never reaches the device at all; the engine does not read its message queue
```

The fix is not a cleverer API call. It is a **separate physical USB device**: an Arduino
Pro Micro (ATmega32u4) running the firmware in `hardware/hid_bridge/`, which is a real USB HID
mouse and keyboard. The host sends line-based commands over a serial port; the device emits
genuine HID reports. At the protocol level it *is* a mouse, so there is nothing to detect.

Two details that matter:

- **Relative movement is kept deliberately.** The Arduino Mouse library has no absolute mode,
  which normally forces a custom HID descriptor. Instead the host closes the loop: send a
  delta, read the real cursor with `GetCursorPos`, send a correction. Two passes land within a
  pixel, and the position is *verified* rather than assumed. Relative motion is also what
  DirectInput wants, so this is not even a compromise.
- **Clicks are held for 70 ms.** The engine samples the mouse once per frame; a press and
  release inside one frame does not exist as far as it is concerned. This was found by recording
  a human's real clicks while they loaded a save — every one held the button for about 70 ms and
  none were double-clicks.

`hardware/hid_bridge/hid_bridge.ino` (firmware) and `hardware/hid_input.py` (host driver).

---

## Three bugs that each cost an evening

These are the reason the project is at 40% and not 90%. All three are worth knowing about.

### 1. Coordinates need dividing by 1.5

The engine renders at **1280x800** but the client area is **853x533**, and it scales pointer
coordinates by `1280/853 = 1.5006`. Every click aimed at a raw client coordinate landed about
50% too low and hit the wrong thing or nothing.

It was found by asking the player to hover over a target and reading `GetCursorPos` back: the
cursor was at client `y=240` while the game highlighted a row at `y≈352`, and `240 × 1.5 = 360`.

```python
SCALE = 1280 / client_width        # 1.5006 on a 853-wide client
cursor_client = render_coordinate / SCALE
```

`mb_agent.py` takes **render-space** coordinates from callers and converts internally.

### 2. My own window was sitting on top of the game

Screenshots were blitted from the game window's DC. That rect is resolved against the *screen*,
so it returns whatever is on top of the window there — including the host application's own
window. Single-pixel sampling settled it:

```
screen (750,300)  = RGB(21,21,23)      dark grey  - the host window
screen (1100,400) = RGB(143,112,70)    parchment  - the game
```

So the left half of the character sheet had been coming back blank *not* because the game failed
to draw it, but because the host window covered it. The same occlusion swallowed clicks: the
cursor was over the host window, which correctly took them.

Fix: capture from `GetDC(NULL)` with `CAPTUREBLT` (`hardware/screen_capture.py`), move the host
window aside, park the game at a fixed position and raise it. Parking it also makes the
coordinate table stable across restarts, which removes a whole class of drift.

### 3. The main menu has no hover feedback, so guessing coordinates never converges

The menu is a static image. A failed click produces no visible signal, so there is no way to tell
a wrong coordinate from a rejected input. The only reliable signal the game gives is *click
something and see whether the frame changes*.

`hardware/menu_map.py` and `hardware/menu_state_sweep.py` sweep an axis, click each step, and
record which screen appeared — checking before every probe that they are still where they think
they are. The first sweep was invalid because it never verified that: one probe opened the
new-game intro, `ESC` did not dismiss it, and eleven consecutive samples then measured the same
stuck screen.

---

## Where this stopped

**The project stops at perception, and perception is the whole problem.**

Everything needed to *act* exists and is verified. What does not exist is any ability to
*understand a screen*. There is no object detection, no OCR, no UI model — coordinates are
hardcoded, one screen at a time, by sweeping. That does not scale to a game with hundreds of
screens, and it is why a single button took an entire evening.

**The honest summary:** I spent the time on the plumbing — input transport, coordinate systems,
window stacking, save format — and those are genuinely the hard parts that kill similar
projects. But I approached perception with 1990s techniques, hand-measuring pixels and sweeping
grids, when the correct answer for this era is a vision model:

```python
frame = look()                                   # works today
target = vision_model(frame, "where is the Finish button?")   # <-- missing
click(target)                                    # works today
```

The infrastructure is exactly what a vision-driven agent needs, and none of it would change.
The missing piece is a solved problem elsewhere. That is the next step, and it is the reason I
stopped rather than continuing to grind coordinates.

**Concretely, what is unfinished:**

- `mb_click` / `mb_key` are wired to the hardware path but have not been re-verified since the
  rewrite — the game was closed before the final test.
- Character creation was reached and skills were allocated (attributes 0→1, life 41→43),
  but the flow was never finished: the page extends past the bottom of the window, and wheel,
  scrollbar drag and paging keys all fail to scroll it.
- Money and the in-game date have not been located in the save file. Both want a differential:
  change the value in game, save twice, diff for the changing int32.
- The player's own party has not been identified among the 41 in the save.
- `tools/verify_mailbox.py` is **inconclusive, not a passing test** — it assumed a memory layout
  that later turned out to be wrong.

---

## What is here

```
mb_agent.py              one CLI, JSON output, everything the plugin calls
mb_bridge.py             window, screenshot, focus, memory reads
hardware/
  hid_bridge/hid_bridge.ino   Arduino Pro Micro firmware: serial -> real USB HID
  hid_input.py                host driver: absolute moves with feedback, keys, wheel
  screen_capture.py           capture the composited desktop, not the window DC
  menu_map.py                 sweep an axis to build a coordinate table
  menu_state_sweep.py         same, but verifies its state before every probe
tools/
  save_state.py               read parties out of a save file
  save_edit.py                move a party in a save file, with backup and verification
dsh-plugin/                   6 DSH model tools wrapping mb_agent.py
docs/
  structured-state-and-named-actions.md   the A+B design, plus the save format as measured
```

### Using the parts that work

```bash
# observation, no game required
python mb_agent.py save-state            # every party, with position and composition
python mb_agent.py save-parties          # the same, indexed for editing

# action on the world, no game required
python mb_agent.py save-move --party 13 --x 12.5 --y -3.25 --dry-run
python mb_agent.py save-move --party 13 --x 12.5 --y -3.25

# live game
python mb_agent.py state                 # window, pid, gold, render scale
python mb_agent.py look --scale 2        # screenshot
python mb_agent.py click 900 420         # RENDER-space coordinates
python mb_agent.py key 0x09              # Tab: leave/back in city and menu views
python mb_agent.py wheel 3
```

### The save format, as measured

```
+0x00  'WRDR'              magic
+0x04  uint32 0x3F3        1011 - the game's own version, self-identifying
+0x0C  uint32 len + bytes  the player name, length-prefixed

party record (fixed-size array, stride 4392 = 0x1128):
  +0x00  float x  +0x04 float y     position, stored three times consecutively
  +0x18  float                      facing
  +0x1C  int                        stack count
  +0x20  (troop_id, count, 0, 0)    repeating every 16 bytes
```

Positions are distinguishable from noise by redundancy: a raw scan finds ~750 float pairs in
the ±105 map range, but only ~41 form stride-8 clusters of three or more. Repeated defaults such
as `(1.0, 1.0)` appear 25 times and are scale/rotation data.

### DSH plugin

`dsh-plugin/` registers six model tools: `mb_state`, `mb_look`, `mb_click`, `mb_key`,
`mb_save_state`, `mb_save_move`. Install by linking the directory into a DSH profile's
`dependencies` and `dsh.profile.bundles`, then restart DSH.

Two traps if you adapt this:

- **Never spawn the child with piped stdio.** Under the DSH sandbox a child that opens an
  inherited named pipe fails with `EPERM`. Open temp files and pass the descriptors as `stdio`.
- **`parameters` is a per-property map** where requiredness is `required: true` on the property
  itself, not a JSON-Schema `required` array.

---

## Limitations

- **Windows only** — Win32 `user32`/`gdi32` throughout.
- **Single-player only**, and specifically a 2008 game with no anti-cheat. Do not point this at
  anything with anti-cheat; a hardware HID device is indistinguishable from a human's mouse by
  design, and that is not a capability worth handing to someone attacking a multiplayer game.
- **Save editing is not realtime.** A change takes effect the next time the save is loaded.
  Good for strategy — moving parties, recruiting, positioning — useless for combat.
- **`read_var` is unreliable.** Only `read_gold` is trustworthy; the module-variable array base
  was never located. The 24-byte signature the bridge scans for marks the player/game state
  struct, not the variable table, and `variables.txt` does not appear to extend the table anyway.
- **The game auto-pauses on focus loss** and ignores input while paused. Every command raises
  the window first for this reason.

---

## License

MIT — see [LICENSE](LICENSE). Vendored reference sources under `third_party/` keep their own
licenses (the original Mount & Blade save editor is MIT; WarBender's status is noted in place).
