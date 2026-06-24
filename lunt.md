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

- **MIDI+MANUAL** — MIDI and the rotary encoder are both live. Turn the encoder to set
  the selected bulb's brightness and press it to choose the target; meanwhile incoming
  MIDI notes/CC also drive the bulbs. The strip shows the selected target (color) and
  its brightness (bar).
- **AUDIO** — MIDI is ignored; the bulbs react to a microphone. Press the encoder to
  pick an animation (its number shows on the strip) and turn the encoder to adjust that
  animation's parameter.

## Controls

### MIDI+MANUAL mode
Encoder:

| Control | Action |
| --- | --- |
| Turn encoder right | Brighter (selected bulb / ALL) |
| Turn encoder left | Dimmer |
| Press encoder | Cycle the target: **ALL → 1 → 2 → 3 → 4 → ALL** (defaults to ALL) |

MIDI (channel **15** by default, `MIDI_CHANNEL` in the sketch):

| Message | Mapping |
| --- | --- |
| Note On / Off `C4`–`D#4` (60–63) | Turn bulbs 1–4 on/off (Note On velocity sets the on-brightness, Note Off turns it fully off) |
| CC 22 / 23 / 24 / 25 | Brightness of bulb 1 / 2 / 3 / 4 |
| CC 27 | Brightness of all bulbs at once |
| CC 26 (≥64 on, <64 off) | Pulse the bulbs on the MIDI beat (clock) |

The strip shows the selected target by color, and the lit length tracks its brightness
(from either source):

| Target | Strip color |
| --- | --- |
| ALL bulbs | ⚪ White |
| Bulb 1 | 🔴 Red |
| Bulb 2 | 🟢 Green |
| Bulb 3 | 🔵 Blue |
| Bulb 4 | 🟠 Amber |

### AUDIO mode

| Control | Action |
| --- | --- |
| Press encoder | Next animation (the strip shows the animation number) |
| Turn encoder | Adjust the current animation's parameter (shown briefly as a blue bar) |

Animations (more to come):

| # | Animation | Encoder parameter |
| --- | --- | --- |
| 1 | Amplitude → brightness | Sensitivity — right = louder affects brightness more |
| 2 | Amplitude → brightness (low-pass) | Cutoff — right opens up the spectrum, left narrows toward bass |

Tune the response in the sketch: `AUDIO_PP_MIN` (noise floor), `AUDIO_PP_FULL_MIN/MAX`
(sensitivity range), `AUDIO_PP_FULL_LPF` (low-pass full-scale), and `AUDIO_RELEASE`
(decay speed).

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

Adjustable knobs are grouped in the `CONFIG / KNOBS` section at the top of the sketch:
mode-switch polarity (`AUDIO_MODE_LEVEL`), encoder step sizes (`ENCODER_STEP` /
`AUDIO_PARAM_STEP`), the note/CC numbers, the clock-pulse default (`gClockAnim`), and
the audio response (`AUDIO_*`).

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
