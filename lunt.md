# Lunt
MIDI-enabled controller for dimmable AC light bulbs, with on-device manual control.

Lunt drives up to four 110/220 V dimmable bulbs through an AC dimmer module. A toggle
switch selects between **MANUAL** (set each bulb's brightness by hand, or over MIDI) and
**ANIMATION** (pick from self-running light shows). An 8-LED NeoPixel strip is the
on-device display — **warm yellow** in MANUAL, **red** in ANIMATION.

## How it works
An Arduino Nano controls the four channels of an [AC Light Dimmer Module](https://robotdyn.com/ac-light-dimmer-module-4-channel-3-3v-5v-logic-ac-50-60hz-220v-110v.html)
via phase-cut dimming (the [Dimmable Light](https://github.com/fabianoriccardi/dimmable-light)
library, synced to the AC zero-cross on D2), and reads MIDI on its hardware serial port.

The toggle switch picks the mode:

- **MANUAL** — the encoder sets brightness; pressing it cycles the target (all bulbs or
  one). MIDI notes/CC also set brightness. The strip is warm yellow and shows the target.
- **ANIMATION** — pick one of 8 self-running animations from a menu and the bulbs play it
  with no input needed. The encoder tunes the animation; a MIDI clock, if present, locks
  it to the beat. The strip is red.

## Controls

### MANUAL mode
| Control | Action |
| --- | --- |
| Turn encoder right / left | Brighter / dimmer (the selected target) |
| Press encoder | Cycle the target: **ALL → 1 → 2 → 3 → 4 → STRIP → TEMPO → SUBDIV** (defaults to ALL) |

After the four bulbs there are three setting targets (turn the encoder to adjust each):

- **STRIP** — the NeoPixel indicator's overall brightness (a warm-yellow bar that dims/
  brightens live). Persists into ANIMATION mode too; floored by `STRIP_BRIGHTNESS_MIN`.
- **TEMPO** — toggle the tempo/clock indicator on/off (right = on, left = off). Shown as
  the far-right pixel lit blue when enabled.
- **SUBDIV** — toggle the subdivision indicator on/off. Shown as the second-from-right
  pixel lit orange when enabled.

No MIDI/clock info is shown on the strip in MANUAL mode itself — only these toggles.

MIDI also drives brightness here (channel **15** by default, `MIDI_CHANNEL` in the sketch):

| Message | Mapping |
| --- | --- |
| Note On / Off `C4`–`D#4` (60–63) | Turn bulbs 1–4 on/off (Note On velocity sets the on-brightness, Note Off turns it fully off) |
| CC 22 / 23 / 24 / 25 | Brightness of bulb 1 / 2 / 3 / 4 |
| CC 27 | Brightness of all bulbs at once |

The strip is **warm yellow**: **ALL** lights every pixel, **Light N** lights the Nth
pixel (counted from the left). While you turn the encoder it shows a brightness bar,
then reverts to the pixel number.

Entering MANUAL mode resets all bulbs to **mid brightness** (`MANUAL_RESET_BRIGHTNESS`),
so nothing stays stuck at whatever level an animation left it.

### ANIMATION mode
The strip is **red**. Each of the 8 animations sits on one of the 8 pixels.

| Control | Action |
| --- | --- |
| Turn encoder (in menu) | Move the cursor over the 8 animations (the lit pixel = selection) |
| Press encoder (in menu) | Enter the highlighted animation |
| Turn encoder (running) | Adjust the animation (speed, or — with a MIDI clock — the beat subdivision) |
| Short press (running) | Re-sync: restart the animation at that instant — tap on the beat to line it up with the music |
| Long hold (running) | Exit back to the selection menu (`LONG_PRESS_MS`) |

| # | Animation | What it does |
| --- | --- | --- |
| 1 | Comet | A bright spot sweeps 1→2→3→4 with a fading tail |
| 2 | Larson | A single bright bulb bounces 1→4→1 (Knight Rider) |
| 3 | Sine sweep | A phase-offset sine wave rolls smoothly across the bulbs |
| 4 | Breathe | All four fade up and down together |
| 5 | Twinkle | Random bulbs sparkle bright then fade |
| 6 | Build & drop | Fills 1→12→123→1234, blackout, repeat |
| 7 | Alternate | 1&3 ↔ 2&4 swap back and forth |
| 8 | Strobe | All bulbs flash together |

**MIDI clock:** when a clock is running, the animation beat-locks and the encoder selects
the subdivision — **1 bar · 1/2 · 1/4 · 1/8 · 1/16** (shown as 1–5 red pixels). With no
clock, the encoder sets a free speed of **20–250 BPM** (shown as a red bar). MIDI notes/CC
are ignored in ANIMATION mode.

**Clock indicators** (right two pixels, while an animation is running):
- **Tempo** (far-right): **blue** steady when no clock; **purple** flashing on each beat
  when a clock is present.
- **Subdivision** (second from right): **orange**, flashing at the selected subdivision
  rate — only when a clock is present.

Each can be turned off via the MANUAL **TEMPO** / **SUBDIV** targets. If the tempo pixel
never turns purple while you send clock, the Arduino isn't receiving it (see
Troubleshooting).

## Hardware

### What you need
- 1x Arduino Nano
- 1x [AC Light Dimmer Module, 4 channel](https://robotdyn.com/ac-light-dimmer-module-4-channel-3-3v-5v-logic-ac-50-60hz-220v-110v.html)
- 4x Dimmable light bulbs
- 4x Bulb sockets
- 4x Light stands
- 1x Rotary encoder with push button (e.g. KY-040)
- 1x 8-LED NeoPixel (WS2812) strip
- 1x 2-way toggle switch
- 1x MIDI female DIN jack
- 2x 220 ohm resistor
- 1x 4.7k resistor
- 1x 1N914 diode
- 1x 6N138 Optocoupler
- Protoboard, wires, soldering iron

> ⚠️ The dimmer module switches mains AC. Wire and enclose the AC side carefully.

### Wiring
All control pins are on the Arduino Nano:

| Arduino pin | Connected to | Notes |
| --- | --- | --- |
| D0 (RX) | MIDI input circuit output | Hardware serial; disconnect while uploading (see below) |
| D2 | Dimmer **SYNC** (zero-cross) | Must be an interrupt pin; required by the dimmer library |
| D9 | Dimmer **CH1** | Bulb 1 gate |
| D7 | Dimmer **CH2** | Bulb 2 gate |
| D5 | Dimmer **CH3** | Bulb 3 gate |
| D6 | Dimmer **CH4** | Bulb 4 gate |
| D3 | Encoder **CLK (A)** | |
| D8 | Encoder **DT (B)** | |
| D12 | Encoder **button** | `INPUT_PULLUP` |
| A1 (D15) | NeoPixel **DIN** | 8 pixels |
| A0 (D14) | Mode **switch** | `INPUT_PULLUP`; other switch terminal to GND |

Power: dimmer module and NeoPixel `VCC`/`GND` to the Nano's `5V`/`GND`; encoder and
switch grounds to `GND`.

**MIDI input** is an opto-isolated circuit (6N138 + 1N914 + 4.7k + 220 ohm) feeding the
Nano's RX (D0). See the [SparkFun MIDI tutorial](https://learn.sparkfun.com/tutorials/midi-tutorial/all)
for the standard input schematic.

**Mode switch logic:** with the internal pull-up, the switch idles HIGH (= MANUAL) and
reads LOW when closed to GND (= ANIMATION). Flip `ANIM_MODE_LEVEL` in the sketch if your
two modes come out reversed.

## Software

### lunt.ino
[Lunt Arduino script.](https://github.com/kbsezginel/polycule/blob/master/lunt/lunt.ino)

Adjustable knobs are grouped in the `CONFIG / KNOBS` section at the top of the sketch.
See [Tunable parameters](#tunable-parameters) for the full list with values and ranges.

### Libraries
Install via the Arduino Library Manager:
- [MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) (FortySevenEffects)
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [Dimmable Light for Arduino](https://github.com/fabianoriccardi/dimmable-light)

Build with the **Arduino AVR Boards** core **1.8.3** — see
[Troubleshooting](#troubleshooting) for why.

### Board settings
- **Board:** Arduino Nano (Tools → Board → Arduino AVR Boards → Arduino Nano)
- **Processor:** ATmega328P, or **ATmega328P (Old Bootloader)** on most CH340 clones
- **Port:** the CH340 / USB-serial port (not a Bluetooth port)

## Tunable parameters
All are `const` (or `#define`) in the `CONFIG / KNOBS` block at the top of `lunt.ino`,
except where noted. Pins are in the [Wiring](#wiring) table. "Range" is the sensible
working range, not the data-type limit.

### Modes & input
| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `ANIM_MODE_LEVEL` | `LOW` | `LOW` / `HIGH` | Switch level that selects ANIMATION (the other position is MANUAL). |
| `ENCODER_STEP` | 8 | 1–32 | Brightness / free-speed change per encoder detent. |
| `BUTTON_DEBOUNCE_MS` | 200 | 50–500 | Minimum ms between accepted encoder-button presses. |
| `LONG_PRESS_MS` | 600 | 300–1500 | Hold time to exit an animation (shorter = a re-sync). |
| `MANUAL_RESET_BRIGHTNESS` | 128 | 0–255 | All bulbs reset to this level when MANUAL mode is entered. |

### MIDI (MANUAL mode)
| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `MIDI_CHANNEL` | 15 | 1–16 | Channel the controller listens on. |
| `NOTE_LIGHT[4]` | 60, 61, 62, 63 | 0–127 each | Note numbers that turn bulbs 1–4 on/off. |
| `NOTE_MIN_BRIGHT` | 40 | 0–255 | Brightness of a note played at the softest velocity. |
| `CC_LIGHT[4]` | 22, 23, 24, 25 | 0–127 each | CC numbers that set bulbs 1–4 brightness. |
| `CC_ALL_BRIGHT` | 27 | 0–127 | CC number that sets all bulbs at once. |

### Strip
| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `NUM_PIXELS` | 8 | 1–~60 | Number of LEDs on the strip; set to match yours. |
| `STRIP_BRIGHTNESS_DEFAULT` | 60 | 0–255 | Indicator brightness at boot (live-adjustable via the MANUAL **STRIP** target). |
| `STRIP_BRIGHTNESS_MIN` | 5 | 0–60 | Floor for the STRIP brightness control so the indicator stays visible. |
| `COLOR_WARM` | (255,130,15) | any RGB | MANUAL-mode strip color (set in `setup()`). |
| `COLOR_RED` | (255,0,0) | any RGB | ANIMATION-mode strip color (set in `setup()`). |
| `MANUAL_BRIGHT_SHOW_MS` | 1200 | 500–3000 | How long the brightness bar shows after a turn before reverting. |

### Animation timing
| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `ANIM_BPM_MIN` | 20 | 5–120 | Free-run speed (BPM) at encoder fully left. One beat = one step / one cycle. |
| `ANIM_BPM_MAX` | 250 | 120–600 | Free-run speed (BPM) at encoder fully right. The knob is linear in BPM. |
| `TAIL_SHIFT` | 2 | 1–4 | Comet/larson/twinkle tail fade: `b -= b >> this` (smaller = longer tail). |
| `CLOCK_TIMEOUT_MS` | 600 | 300–2000 | Clock is treated as absent (→ free speed) after this gap. |
| `CLOCK_BLINK_MS` | 90 | 30–250 | How long the purple clock indicator stays bright on each beat. |

Beat-lock subdivisions are fixed in `SUBDIV_BEATS` (1 bar, 1/2, 1/4, 1/8, 1/16).

## Troubleshooting

### Compile fails: `multiple definition of 'std::nothrow'`
The full error looks like `new.cpp.o ... multiple definition of 'std::nothrow' ...
ArduinoSTL/new_handler.cpp.o ... first defined here`.

**Cause:** the Dimmable Light library depends on **ArduinoSTL**, which redefines
`operator new` / `std::nothrow` already provided by the **Arduino AVR Boards** core
1.8.4 and newer, so the linker sees two definitions.

**Fix:** downgrade the core to **1.8.3**, the last version that links cleanly with
ArduinoSTL: Tools → Board → Boards Manager → search "Arduino AVR Boards" → pick version
**1.8.3** → Install. Re-select the Nano afterwards and recompile.

### Upload fails: `avrdude: stk500_recv(): programmer is not responding`
**Cause #1 — the MIDI input is on RX.** MIDI uses the Nano's hardware serial (D0/D1),
the same UART the bootloader uses to receive the program. If the MIDI input circuit is
connected to **RX (D0)**, it blocks the upload.
**Fix:** disconnect whatever is wired to **D0 (RX)** before uploading, then reconnect it
afterward. (This is inherent to using hardware serial for MIDI — you'll do it every flash.)

**Cause #2 — wrong bootloader.** Most CH340-based Nano clones ship with the old
bootloader.
**Fix:** select **Tools → Processor → ATmega328P (Old Bootloader)** and upload again.

Also check the **Serial Monitor is closed** (it holds the port) and that the correct
**CH340/USB-serial port** is selected.

### Brightness jumps around when turning the encoder
Mechanical rotary encoders bounce, and EMI from the nearby AC dimmer makes it worse.
The sketch already decodes the encoder with a quadrature state table that rejects
bounce. If it still twitches, add a **100 nF capacitor from each encoder pin (CLK, DT)
to GND** for hardware debouncing.

### Encoder direction or strip fill is backwards
Flip the `right` test in `handleEncoder()` to reverse the knob direction; the strip's
left/right mapping is in `fillPixels()` / `lightOnePixel()`.

### MIDI clock isn't detected (animations don't beat-lock)
First check the **clock indicator**: enter an animation (ANIMATION mode → press the
encoder) and watch the far-right pixel — it turns purple and flashes on the beat when a
clock is arriving. If it stays blue (or dark):
- **Enable clock output** on the source. Most DAWs/keyboards don't send MIDI clock by
  default — turn on "send MIDI clock / sync" for the correct port.
- **Press play.** Many sources only send clock while the transport is running; just
  changing the tempo number sends nothing.
- Confirm the MIDI input circuit is wired to **RX (D0)** and working (notes work in
  MANUAL mode is a good check).

If the indicator *is* purple but tempo seems to do nothing: make sure you've **entered**
an animation (pressed the encoder), since the clock only drives a running animation, and
that you're watching a tempo-sensitive one (e.g. comet or strobe).
