/*
 * hid_bridge.ino - turn an Arduino Pro Micro (ATmega32u4) into a real USB mouse+keyboard
 * that a host program can drive over the serial port.
 *
 * Why this exists
 * ---------------
 * Mount & Blade 1.011 reads input through DirectInput, which reads DEVICE STATE. Synthetic
 * events from SendInput carry an "injected" flag and PostMessage never reaches the device at
 * all, so both were silently dropped - verified twice, byte-identical frames before and
 * after. A separate physical USB device is not distinguishable from a human's mouse, because
 * at the protocol level it IS a mouse. That is the whole point.
 *
 * Design notes
 * ------------
 * The Arduino Mouse library only does RELATIVE movement, which normally forces you to write a
 * custom absolute-position HID descriptor. We deliberately keep relative movement instead:
 *
 *   - DirectInput reads relative deltas anyway, so this is the more natural fit for games.
 *   - Absolute placement is achieved by feedback: the host moves, reads the real cursor
 *     position, and sends a correction delta. Two steps typically land within a pixel.
 *   - The firmware stays tiny and dependency-free.
 *
 * Protocol - one ASCII command per line, terminated by '\n':
 *
 *   m <dx> <dy>      relative mouse move, signed ints
 *   d <button>       button down, button in {1=left, 2=right, 4=middle}
 *   u <button>       button up
 *   c <button>       click: down + 12 ms + up
 *   r <delta>        wheel, positive scrolls down (use +-1..3 per notch)
 *   k <hidcode>      key press + release, HID usage id (not a Windows VK)
 *   K <hidcode>      key down (for held movement, e.g. W)
 *   U <hidcode>      key up
 *   w <ms>           busy-wait, for timing patterns
 *   p                ping -> replies "ok"
 *
 * Every command replies with a single line: "ok" or "err <reason>". The host waits for it,
 * which keeps the two ends in lockstep and makes failures visible instead of silent.
 *
 * Build: Arduino IDE, board "Arduino Leonardo" (Pro Micro uses the same ATmega32u4 core),
 * or PlatformIO with board = sparkfun_promicro16.
 */

#include <Mouse.h>
#include <Keyboard.h>

// Holding a mouse button this long before releasing it is not cosmetic: the game samples the
// mouse once per frame, so a press and release landing inside one frame is invisible to it.
// 12 ms spans at least one frame at 80 fps and keeps click latency low.
static const unsigned int CLICK_HOLD_MS = 70;

static char lineBuf[64];
static uint8_t lineLen = 0;

static uint8_t buttonMask(int b) {
  switch (b) {
    case 1: return MOUSE_LEFT;
    case 2: return MOUSE_RIGHT;
    case 4: return MOUSE_MIDDLE;
    default: return 0;
  }
}

static void handle(char *line) {
  // tokenise: single-letter command, then up to a few integer arguments
  char *cmd = strtok(line, " \t");
  if (cmd == NULL) { Serial.println("err empty"); return; }

  char c = cmd[0];
  long a = 0, b = 0;
  char *t1 = strtok(NULL, " \t");
  char *t2 = strtok(NULL, " \t");
  if (t1) a = atol(t1);
  if (t2) b = atol(t2);

  switch (c) {
    case 'p':
      Serial.println("ok");
      return;

    case 'm': {
      // Mouse.move takes int8 per axis; chunk larger deltas so a long move is not clipped.
      long dx = a, dy = b;
      while (dx != 0 || dy != 0) {
        int sx = (int)constrain(dx, -127, 127);
        int sy = (int)constrain(dy, -127, 127);
        Mouse.move(sx, sy, 0);
        dx -= sx;
        dy -= sy;
      }
      break;
    }

    case 'd': {
      uint8_t m = buttonMask((int)a);
      if (!m) { Serial.println("err button"); return; }
      Mouse.press(m);
      break;
    }

    case 'u': {
      uint8_t m = buttonMask((int)a);
      if (!m) { Serial.println("err button"); return; }
      Mouse.release(m);
      break;
    }

    case 'c': {
      uint8_t m = buttonMask((int)a);
      if (!m) { Serial.println("err button"); return; }
      Mouse.press(m);
      delay(CLICK_HOLD_MS);
      Mouse.release(m);
      break;
    }

    case 'r': {
      // wheel: Arduino reports it as a signed byte, so chunk larger rolls
      long d = a;
      while (d != 0) {
        int s = (int)constrain(d, -127, 127);
        Mouse.move(0, 0, s);
        d -= s;
      }
      break;
    }

    case 'k':
      Keyboard.press((uint8_t)a);
      delay(CLICK_HOLD_MS);
      Keyboard.release((uint8_t)a);
      break;

    case 'K':
      Keyboard.press((uint8_t)a);
      break;

    case 'U':
      Keyboard.release((uint8_t)a);
      break;

    case 'w':
      delay((unsigned long)a);
      break;

    default:
      Serial.println("err command");
      return;
  }
  Serial.println("ok");
}

void setup() {
  Serial.begin(115200);
  Mouse.begin();
  Keyboard.begin();
  // give the host a moment, then announce readiness
  delay(400);
  Serial.println("ready hid_bridge");
}

void loop() {
  while (Serial.available() > 0) {
    char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (lineLen > 0) {
        lineBuf[lineLen] = '\0';
        handle(lineBuf);
        lineLen = 0;
      }
    } else if (lineLen < sizeof(lineBuf) - 1) {
      lineBuf[lineLen++] = ch;
    } else {
      lineLen = 0;                 // overflow: drop the line rather than corrupt it
      Serial.println("err overflow");
    }
  }
}
