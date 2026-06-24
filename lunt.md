# Lunt
MIDI-enabled controller for dimmable AC light bulbs, with on-device manual control.

Lunt drives up to four 110/220 V dimmable bulbs through an AC dimmer module. A toggle
switch selects between **MIDI+MANUAL** (drive the bulbs from a keyboard/DAW and/or by
hand with the rotary encoder) and **AUDIO** (react to a microphone). An 8-LED NeoPixel
strip is the on-device display.

## How it works
An Arduino Nano reads incoming MIDI on its hardware serial port and controls the four
channels of an [AC Light Dimmer Module](https://robotdyn.com/ac-light-dimmer-module-4-channel-3-3v-5v-logic-ac-50-60hz-220v-110v.html)
via phase-cut dimming (the [Dimmable Light](https://github.com/fabianoriccardi/dimmable-light)
library, synced to the AC zero-cross on D2).

The toggle switch picks the mode:

- **MIDI+MANUAL** — MIDI and the rotary encoder are both live. Press the encoder to
  choose a target (a bulb / all / a self-running animation) and turn it to set
  brightness or, on an animation, its speed; meanwhile incoming MIDI notes/CC also
  drive the bulbs. The strip shows the selected target and its brightness, or the
  animation number.
- **AUDIO** — MIDI is ignored; the bulbs react to a microphone. Press the encoder to
  pick an animation (its number shows on the strip) and turn the encoder to adjust that
  animation's parameter.

## Controls

### MIDI+MANUAL mode
Pressing the encoder cycles through the targets — the five brightness controls first,
then the self-running animations:

**ALL → 1 → 2 → 3 → 4 → [animations] → ALL**

| Control | On a brightness target (ALL / 1–4) | On an animation |
| --- | --- | --- |
| Turn encoder right | Brighter | Faster |
| Turn encoder left | Dimmer | Slower |

MIDI (channel **15** by default, `MIDI_CHANNEL` in the sketch) — active on the brightness
targets; an animation overrides the bulbs while it's selected:

| Message | Mapping |
| --- | --- |
| Note On / Off `C4`–`D#4` (60–63) | Turn bulbs 1–4 on/off (Note On velocity sets the on-brightness, Note Off turns it fully off) |
| CC 22 / 23 / 24 / 25 | Brightness of bulb 1 / 2 / 3 / 4 |
| CC 27 | Brightness of all bulbs at once |
| CC 26 (≥64 on, <64 off) | Pulse the bulbs on the MIDI beat (clock) |

On a brightness target the strip shows it by color, with the lit length tracking its
brightness (from either source); on an animation it shows the animation number:

| Target | Strip color |
| --- | --- |
| ALL bulbs | ⚪ White |
| Bulb 1 | 🔴 Red |
| Bulb 2 | 🟢 Green |
| Bulb 3 | 🔵 Blue |
| Bulb 4 | 🟠 Amber |

Self-running animations (no audio/MIDI needed — the encoder turn sets the speed):

| # | Animation | What it does |
| --- | --- | --- |
| 1 | Comet | A bright spot sweeps 1→2→3→4 with a fading tail |
| 2 | Sine sweep | A phase-offset sine wave rolls smoothly across the bulbs |
| 3 | Breathe | All four fade up and down together |
| 4 | Larson scanner | A single bright bulb bounces 1→4→1 (Knight Rider) |
| 5 | Twinkle | Random bulbs sparkle bright then fade |

### AUDIO mode

| Control | Action |
| --- | --- |
| Press encoder | Next animation (the strip shows the animation number) |
| Turn encoder | Adjust the current animation's parameter (shown briefly as a blue bar) |

Animations (more to come). The strip shows the number; the encoder turn adjusts the
listed parameter:

| # | Animation | What it does | Encoder parameter |
| --- | --- | --- | --- |
| 1 | Amplitude → brightness | All bulbs follow overall loudness | Sensitivity — right = louder affects brightness more |
| 2 | Amplitude → brightness (low-pass) | Loudness of a low-pass-filtered signal | Cutoff — right opens up the spectrum, left narrows toward bass |
| 3 | Spectrum split | Bass→bulb 1, low-mid→2, high-mid→3, treble→4 | Sensitivity — right = quieter bands light up |
| 4 | Beat pulse | Detects bass-energy spikes (kicks) and pulses all bulbs, then fades | Sensitivity — right = triggers more easily |
| 5 | Breathing | All bulbs follow a heavily smoothed loudness envelope (ambient) | Smoothing — right = slower / smoother |
| 6 | Sidechain / duck | Bulbs sit bright and dip on loud hits (pumping look) | Depth — right = deeper dip |

Tune the response in the sketch (`CONFIG / KNOBS` section): `AUDIO_PP_MIN` (noise floor),
`AUDIO_PP_FULL_*` (full-scale loudness per animation), `AUDIO_RELEASE` (decay speed),
the `SPLIT_ALPHA*` crossovers (spectrum bands), and the `BEAT_*` constants (beat
detection).

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
- 1x Microphone amplifier module (MAX4466 or MAX9814) — for AUDIO mode
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
| A2 | Mic amp module **OUT** | Audio-reactive input; module `VCC`/`GND` to `5V`/`GND` |

Power: dimmer module and NeoPixel `VCC`/`GND` to the Nano's `5V`/`GND`; encoder and
switch grounds to `GND`.

**Audio input:** use a mic amp module (MAX4466/MAX9814) whose output is already biased
to ~2.5 V — wire its OUT straight to A2. Do **not** connect a raw line/headphone signal
directly: it's AC (swings negative) and would need a DC-bias + coupling-cap network
first. Keep audio wiring away from the AC/dimmer side to avoid picking up noise.

**MIDI input** is an opto-isolated circuit (6N138 + 1N914 + 4.7k + 220 ohm) feeding the
Nano's RX (D0). See the [SparkFun MIDI tutorial](https://learn.sparkfun.com/tutorials/midi-tutorial/all)
for the standard input schematic.

**Mode switch logic:** with the internal pull-up, the switch idles HIGH (= MIDI+MANUAL)
and reads LOW when closed to GND (= AUDIO). Flip `AUDIO_MODE_LEVEL` in the sketch if
your two modes come out reversed.

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
All of these are `const` (or `#define`) at the top of `lunt.ino` in the `CONFIG / KNOBS`
section, except where noted. Pin assignments live in the [Wiring](#wiring) table.
"Range" is the sensible working range, not the data-type limit.

### Modes & input
| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `AUDIO_MODE_LEVEL` | `LOW` | `LOW` / `HIGH` | Switch level that selects AUDIO mode (the other position is MIDI+MANUAL). Flip if reversed. |
| `ENCODER_STEP` | 8 | 1–32 | Brightness (or animation-speed) change per detent in MIDI+MANUAL mode. |
| `AUDIO_PARAM_STEP` | 8 | 1–32 | Animation-parameter change per encoder detent in AUDIO mode. |
| `BUTTON_DEBOUNCE_MS` | 200 | 50–500 | Minimum ms between accepted encoder-button presses. |

### MIDI+MANUAL animations
| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `MANUAL_FRAME_MIN_MS` | 12 | 5–50 | Frame time at full speed (smaller = faster). |
| `MANUAL_FRAME_MAX_MS` | 180 | 80–500 | Frame time at the slowest speed setting. |
| `MANUAL_PHASE_STEP` | 0.20 | 0.05–0.6 | Radians the sine/breathe phase advances per frame. |
| `MANUAL_TAIL_SHIFT` | 2 | 1–4 | Comet/larson/twinkle tail fade: `b -= b >> this` (smaller = longer tail). |

### MIDI mapping
| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `MIDI_CHANNEL` | 15 | 1–16 | Channel the controller listens on. |
| `NOTE_LIGHT[4]` | 60, 61, 62, 63 | 0–127 each | Note numbers that turn bulbs 1–4 on/off. |
| `NOTE_MIN_BRIGHT` | 40 | 0–255 | Brightness of a note played at the softest velocity. |
| `CC_LIGHT[4]` | 22, 23, 24, 25 | 0–127 each | CC numbers that set bulbs 1–4 brightness. |
| `CC_ALL_BRIGHT` | 27 | 0–127 | CC number that sets all bulbs at once. |
| `CC_CLOCK_ANIM` | 26 | 0–127 | CC number that toggles the clock beat-pulse (≥64 on). |
| `gClockAnim` | `false` | `true` / `false` | Whether the MIDI clock pulses the bulbs by default. |

### NeoPixel strip
| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `NUM_PIXELS` | 8 | 1–~60 | Number of LEDs on the strip; set to match yours. |
| `ledStrip.setBrightness()` | 60 | 0–255 | Overall strip brightness cap (set inline in `setup()`). |

### Audio — general
| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `AUDIO_PIN` | `A2` | A2/A3/A6/A7 | Analog pin the mic amp output feeds. |
| `AUDIO_WINDOW_MS` | 25 | 10–50 | Window over which loudness is measured / outputs update. |
| `AUDIO_PP_MIN` | 20 | 5–100 | Peak-to-peak (ADC counts) below this is treated as silence. |
| `AUDIO_RELEASE` | 6 | 1–40 | How fast brightness falls per window (VU release). |
| `AUDIO_PARAM_SHOW_MS` | 1200 | 500–3000 | How long the parameter bar shows on the strip after a turn. |

### Audio — per-animation sensitivity (full-scale loudness)
Lower value = more sensitive (less sound reaches full brightness).

| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `AUDIO_PP_FULL_MAX` | 800 | 200–1023 | Amp→brightness full-scale at minimum sensitivity (anim 1). |
| `AUDIO_PP_FULL_MIN` | 60 | 20–400 | Amp→brightness full-scale at maximum sensitivity (anim 1). |
| `AUDIO_PP_FULL_LPF` | 250 | 50–1023 | Full-scale for the low-pass animation (anim 2). |
| `AUDIO_PP_FULL_SPLIT_MAX` | 400 | 100–800 | Spectrum per-band full-scale at min sensitivity (anim 3). |
| `AUDIO_PP_FULL_SPLIT_MIN` | 60 | 20–300 | Spectrum per-band full-scale at max sensitivity (anim 3). |
| `AUDIO_PP_FULL_BREATH` | 400 | 100–1023 | Loudness full-scale for breathing (anim 5). |
| `AUDIO_PP_FULL_DUCK` | 400 | 100–1023 | Loudness full-scale for sidechain duck (anim 6). |

### Audio — filters & beat detection
Filter `*_ALPHA` values are one-pole coefficients (alpha/256 per sample): smaller =
lower cutoff / slower.

| Variable | Value | Range | Description |
| --- | --- | --- | --- |
| `DC_ALPHA` | 2 | 1–16 | DC blocker speed (removes the ~2.5 V mic bias). |
| `SPLIT_ALPHA1` | 8 | 1–256 | Bass \| low-mid crossover (anim 3). |
| `SPLIT_ALPHA2` | 40 | 1–256 | Low-mid \| high-mid crossover (anim 3). |
| `SPLIT_ALPHA3` | 140 | 1–256 | High-mid \| treble crossover (anim 3). |
| `BEAT_BASS_ALPHA` | 30 | 1–256 | Bass band used for beat detection (anim 4). |
| `BEAT_AVG_ALPHA` | 24 | 1–128 | How fast the beat energy average adapts. |
| `BEAT_PP_MIN` | 25 | 5–200 | Ignore beats with bass energy below this. |
| `BEAT_DECAY` | 28 | 1–128 | How fast the beat pulse fades per window. |
| `SIDE_RELEASE` | 8 | 1–64 | How fast sidechain duck recovers per window (anim 6). |

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
Swap the two `ENCODER_STEP` signs in `handleEncoder()` to reverse the knob direction;
the brightness bar fill direction is set in `fillStrip()`.
