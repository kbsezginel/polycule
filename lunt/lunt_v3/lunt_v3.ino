// ----------------------------------------------------------------------------------
// POLYCULE | LUNT | MIDI-enabled light controller
// ----------------------------------------------------------------------------------
// Arduino Nano + RobotDyn 4-channel AC dimmer + rotary encoder + 8px NeoPixel strip.
//
// MODE is chosen by the SWITCH:
//   * MIDI mode   -> listen to MIDI: notes turn lights ON/OFF, CC sets brightness,
//                    MIDI clock drives animations. Strip blinks in time with clock.
//   * MANUAL mode -> rotary encoder sets brightness of the selected light.
//                    Press the encoder to cycle: Light1 -> 2 -> 3 -> 4 -> ALL.
//                    Strip shows the selected light (color) + its brightness (bar).
// ----------------------------------------------------------------------------------
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>
#include "dimmable_light.h"

// ================================ CONFIG / KNOBS ==================================
// Which switch level means "MIDI mode". Switch is INPUT_PULLUP, so it idles HIGH and
// reads LOW when closed to GND. If your two modes feel reversed, flip HIGH <-> LOW.
#define MIDI_MODE_LEVEL   HIGH

// Encoder feel: brightness change (0-255) applied per detent (one click of the knob).
const int  ENCODER_STEP     = 8;

// MIDI note -> light mapping (one note per light). Note ON sets brightness from
// velocity, Note OFF turns the light fully off.
const byte NOTE_LIGHT[4]    = {60, 61, 62, 63};   // C4, C#4, D4, D#4
const byte NOTE_MIN_BRIGHT  = 40;                 // softest visible "on" level

// MIDI CC -> light brightness mapping, plus a couple of control CCs.
const byte CC_LIGHT[4]      = {22, 23, 24, 25};
const byte CC_ALL_BRIGHT    = 27;                 // brightness of all lights at once
const byte CC_CLOCK_ANIM    = 26;                 // >=64 enables clock light animation

// Set true to let the MIDI clock drive the LIGHTS by default (otherwise the clock
// only animates the NeoPixel strip and notes/CC keep direct control of the lights).
bool gClockAnim = false;

// -------------------------------- CONTROLLER IO -----------------------------------
const byte SWITCH_PIN     = A0;   // mode switch  (D14)
const byte LED_PIN        = A1;   // NeoPixel data (D15)
const byte RE_CLK_PIN     = 3;    // encoder A / CLK (INT1)
const byte RE_DT_PIN      = 8;    // encoder B / DT
const byte RE_BUTTON_PIN  = 12;   // encoder push button
const unsigned long BUTTON_DEBOUNCE_MS = 200;

// -------------------------------- DIMMER SETUP ------------------------------------
const byte DIM_SYNC_PIN   = 2;    // zero-cross sync (INT0) - required by the library
const byte DIM1_PIN       = 9;
const byte DIM2_PIN       = 7;
const byte DIM3_PIN       = 5;
const byte DIM4_PIN       = 6;

DimmableLight dimLight1(DIM1_PIN);
DimmableLight dimLight2(DIM2_PIN);
DimmableLight dimLight3(DIM3_PIN);
DimmableLight dimLight4(DIM4_PIN);
DimmableLight* gLights[4] = {&dimLight1, &dimLight2, &dimLight3, &dimLight4};

const byte NUM_LIGHTS = 4;
byte gBrightness[NUM_LIGHTS] = {0, 0, 0, 0};   // last brightness sent to each light

// ---------------------------------- MIDI SETUP ------------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 15

// -------------------------------- NEOPIXEL SETUP ----------------------------------
const byte NUM_PIXELS = 8;
Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// -------------------------------- STATE / RUNTIME ---------------------------------
bool gMidiMode = false;            // current mode (set in setup)
bool gStripDirty = true;           // manual-mode strip needs redraw

// Manual-mode selection: 0..3 = single light, 4 = ALL lights.
byte gSelected = 0;
bool gButtonLast = false;
unsigned long gButtonLastTime = 0;

// Rotary encoder: polled quadrature state-table decoder (Ben Buxton style). It only
// emits a step on a complete, valid detent transition, so contact bounce and noise
// are rejected instead of being counted as random jumps.
#define R_START     0x0
#define R_CW_FINAL  0x1
#define R_CW_BEGIN  0x2
#define R_CW_NEXT   0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT  0x6
#define DIR_NONE    0x0
#define DIR_CW      0x10
#define DIR_CCW     0x20
const unsigned char ENCODER_TABLE[7][4] = {
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
unsigned char gEncoderState = R_START;

// MIDI clock / beat tracking.
const byte PPQN = 24;              // MIDI clock pulses per quarter note
byte gClockCount = 0;
bool gClockOn = false;             // clock light-animation toggle state
byte gBeatColorIdx = 0;            // color cycled per beat on the strip
bool gBeatFlashActive = false;
unsigned long gBeatFlashStart = 0;
const unsigned long BEAT_FLASH_MS = 70;

// ----------------------------------------------------------------------------------
// >x< SETUP >x<
// ----------------------------------------------------------------------------------
void setup() {
  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleClock(handleClock);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleContinue(handleStart);   // resume == restart our beat counter
  MIDI.setHandleStop(handleStop);

  pinMode(RE_CLK_PIN, INPUT_PULLUP);
  pinMode(RE_DT_PIN, INPUT_PULLUP);
  pinMode(RE_BUTTON_PIN, INPUT_PULLUP);

  pinMode(SWITCH_PIN, INPUT_PULLUP);

  ledStrip.begin();
  ledStrip.setBrightness(60);   // overall strip cap, 0-255
  clearStrip();

  DimmableLight::setSyncPin(DIM_SYNC_PIN);
  DimmableLight::begin();

  gMidiMode = (digitalRead(SWITCH_PIN) == MIDI_MODE_LEVEL);
  onModeChange();
}

// ----------------------------------------------------------------------------------
// >x< LOOP >x<
// ----------------------------------------------------------------------------------
void loop() {
  bool midiMode = (digitalRead(SWITCH_PIN) == MIDI_MODE_LEVEL);
  if (midiMode != gMidiMode) {
    gMidiMode = midiMode;
    onModeChange();
  }

  if (gMidiMode) {
    MIDI.read();          // fires the note/CC/clock handlers above
    updateBeatFlash();    // clears the strip flash after BEAT_FLASH_MS
  } else {
    handleEncoder();
    handleEncoderButton();
    if (gStripDirty) {
      drawManualStrip();
      gStripDirty = false;
    }
  }
}

// Called once whenever the switch flips between modes.
void onModeChange() {
  if (gMidiMode) {
    gClockCount = 0;
    gBeatFlashActive = false;
    clearStrip();
  } else {
    gEncoderState = R_START;   // start the decoder fresh
    gStripDirty = true;        // redraw selection + brightness bar
  }
}

// ----------------------------------------------------------------------------------
// DIMMER HELPERS
// ----------------------------------------------------------------------------------
void setLight(byte index, int value) {
  value = constrain(value, 0, 255);
  gBrightness[index] = (byte)value;
  gLights[index]->setBrightness((byte)value);
}

void setAllLights(int value) {
  for (byte i = 0; i < NUM_LIGHTS; i++) {
    setLight(i, value);
  }
}

// ----------------------------------------------------------------------------------
// ROTARY ENCODER (MANUAL MODE)
// ----------------------------------------------------------------------------------
// Poll both encoder pins and feed them through the state table. Returns DIR_CW,
// DIR_CCW once per detent, or DIR_NONE for intermediate / bounce transitions.
unsigned char readEncoder() {
  unsigned char pinState = (digitalRead(RE_DT_PIN) << 1) | digitalRead(RE_CLK_PIN);
  gEncoderState = ENCODER_TABLE[gEncoderState & 0x0F][pinState];
  return gEncoderState & 0x30;
}

void handleEncoder() {
  unsigned char dir = readEncoder();
  if (dir == DIR_NONE) {
    return;
  }
  int delta = (dir == DIR_CW) ? ENCODER_STEP : -ENCODER_STEP;
  if (gSelected == NUM_LIGHTS) {            // ALL selected
    for (byte i = 0; i < NUM_LIGHTS; i++) {
      setLight(i, gBrightness[i] + delta);
    }
  } else {
    setLight(gSelected, gBrightness[gSelected] + delta);
  }
  gStripDirty = true;
}

void handleEncoderButton() {
  bool pressed = (digitalRead(RE_BUTTON_PIN) == LOW);
  if (pressed && !gButtonLast && (millis() - gButtonLastTime > BUTTON_DEBOUNCE_MS)) {
    gSelected = (gSelected + 1) % (NUM_LIGHTS + 1);   // 0..3 lights, 4 = ALL
    gButtonLastTime = millis();
    gStripDirty = true;
  }
  gButtonLast = pressed;
}

// ----------------------------------------------------------------------------------
// NEOPIXEL HELPERS
// ----------------------------------------------------------------------------------
void clearStrip() {
  for (byte i = 0; i < NUM_PIXELS; i++) {
    ledStrip.setPixelColor(i, 0);
  }
  ledStrip.show();
}

void fillStrip(uint32_t color, byte litCount) {
  for (byte i = 0; i < NUM_PIXELS; i++) {
    ledStrip.setPixelColor(i, i < litCount ? color : 0);
  }
  ledStrip.show();
}

// Color used to indicate which light is selected in manual mode.
uint32_t selectionColor(byte sel) {
  switch (sel) {
    case 0:  return ledStrip.Color(255,   0,   0);  // light 1 - red
    case 1:  return ledStrip.Color(  0, 255,   0);  // light 2 - green
    case 2:  return ledStrip.Color(  0,   0, 255);  // light 3 - blue
    case 3:  return ledStrip.Color(255, 150,   0);  // light 4 - amber
    default: return ledStrip.Color(255, 255, 255);  // ALL      - white
  }
}

// Manual mode: color = selected light, lit pixels = its brightness.
void drawManualStrip() {
  byte b;
  if (gSelected == NUM_LIGHTS) {
    int sum = 0;
    for (byte i = 0; i < NUM_LIGHTS; i++) sum += gBrightness[i];
    b = sum / NUM_LIGHTS;
  } else {
    b = gBrightness[gSelected];
  }
  byte litCount = map(b, 0, 255, 0, NUM_PIXELS);
  fillStrip(selectionColor(gSelected), litCount);
}

uint32_t colorWheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85)  return ledStrip.Color(255 - pos * 3, 0, pos * 3);
  if (pos < 170) { pos -= 85;  return ledStrip.Color(0, pos * 3, 255 - pos * 3); }
  pos -= 170;
  return ledStrip.Color(pos * 3, 255 - pos * 3, 0);
}

// ----------------------------------------------------------------------------------
// MIDI NOTE / CC HANDLERS (MIDI MODE)
// ----------------------------------------------------------------------------------
void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (velocity == 0) {            // running-status note-off
    handleNoteOff(channel, pitch, velocity);
    return;
  }
  for (byte i = 0; i < NUM_LIGHTS; i++) {
    if (pitch == NOTE_LIGHT[i]) {
      setLight(i, map(velocity, 1, 127, NOTE_MIN_BRIGHT, 255));
      break;
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  for (byte i = 0; i < NUM_LIGHTS; i++) {
    if (pitch == NOTE_LIGHT[i]) {
      setLight(i, 0);             // note off = light fully off
      break;
    }
  }
}

void handleControlChange(byte channel, byte number, byte value) {
  for (byte i = 0; i < NUM_LIGHTS; i++) {
    if (number == CC_LIGHT[i]) {
      setLight(i, map(value, 0, 127, 0, 255));
      return;
    }
  }
  if (number == CC_ALL_BRIGHT) {
    setAllLights(map(value, 0, 127, 0, 255));
  } else if (number == CC_CLOCK_ANIM) {
    gClockAnim = (value >= 64);
  }
}

// ----------------------------------------------------------------------------------
// MIDI CLOCK HANDLERS (MIDI MODE)
// 24 clock pulses per quarter note. We act once per beat.
// ----------------------------------------------------------------------------------
void handleClock() {
  if (!gMidiMode) return;
  if (++gClockCount >= PPQN) {
    gClockCount = 0;
    onBeat();
  }
}

void handleStart() {
  gClockCount = 0;
  gClockOn = false;
}

void handleStop() {
  gClockCount = 0;
  if (gMidiMode) clearStrip();
}

void onBeat() {
  // Strip: flash a new color each beat as a tempo indicator.
  fillStrip(colorWheel(gBeatColorIdx), NUM_PIXELS);
  gBeatFlashActive = true;
  gBeatFlashStart = millis();
  gBeatColorIdx += 32;

  // Optional: also drive the lights from the clock (simple on/off pulse per beat).
  // Add more patterns here as the project grows.
  if (gClockAnim) {
    gClockOn = !gClockOn;
    setAllLights(gClockOn ? 255 : 0);
  }
}

// Turn the beat flash off after a short time (called every MIDI-mode loop).
void updateBeatFlash() {
  if (gBeatFlashActive && (millis() - gBeatFlashStart > BEAT_FLASH_MS)) {
    clearStrip();
    gBeatFlashActive = false;
  }
}
