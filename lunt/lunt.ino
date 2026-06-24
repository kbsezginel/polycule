// ----------------------------------------------------------------------------------
// POLYCULE | LUNT | MIDI-enabled light controller
// ----------------------------------------------------------------------------------
// Arduino Nano + RobotDyn 4-channel AC dimmer + rotary encoder + 8px NeoPixel strip.
//
// The SWITCH chooses one of two modes:
//
//   * MANUAL (strip = warm yellow)
//       - Encoder press : cycle the target ALL -> 1 -> 2 -> 3 -> 4.
//       - Encoder turn  : set that target's brightness (MIDI notes/CC also set it).
//       - Strip: ALL = all pixels, Light N = the Nth pixel. Turning briefly shows a
//         brightness bar, then reverts to the pixel-number display.
//
//   * ANIMATION (strip = red)
//       - Selection menu: turn to move the cursor over the 8 animations (one pixel each),
//         press to enter, press again to exit back to the menu.
//       - In an animation: encoder sets its parameter (shown on the strip). MIDI notes/CC
//         are ignored. If a MIDI clock is present the animation beat-locks and the encoder
//         picks the subdivision (1 bar / 1/2 / 1/4 / 1/8 / 1/16).
//       - Animations: comet, larson, sine sweep, breathe, twinkle, build, alternate, strobe.
// ----------------------------------------------------------------------------------
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>
#include "dimmable_light.h"

// ================================ CONFIG / KNOBS ==================================
// Switch level that selects ANIMATION mode. The switch is INPUT_PULLUP, so it idles
// HIGH (= MANUAL) and reads LOW when closed to GND (= ANIMATION). Flip if reversed.
#define ANIM_MODE_LEVEL    LOW

const int  ENCODER_STEP   = 8;     // brightness / free-speed change per encoder detent
const unsigned long BUTTON_DEBOUNCE_MS = 200;

// MIDI note -> light mapping (MANUAL mode). Note ON sets brightness from velocity,
// Note OFF turns the light fully off.
const byte NOTE_LIGHT[4]   = {60, 61, 62, 63};   // C4, C#4, D4, D#4
const byte NOTE_MIN_BRIGHT = 40;                 // softest visible "on" level
// MIDI CC -> light brightness mapping (MANUAL mode).
const byte CC_LIGHT[4]     = {22, 23, 24, 25};
const byte CC_ALL_BRIGHT   = 27;                 // brightness of all lights at once

// Animation timing. Free-run unit (encoder speed) ranges between these; one "unit" is
// one step (stepwise anims) or one cycle (continuous anims).
const unsigned long UNIT_MS_MIN = 80;            // fastest (encoder fully right)
const unsigned long UNIT_MS_MAX = 2000;          // slowest (encoder fully left)
const byte TAIL_SHIFT = 2;                       // comet/larson/twinkle fade: b -= b >> this

// Strip feedback timing.
const unsigned long MANUAL_BRIGHT_SHOW_MS = 1200;  // brightness bar shown after a turn

// MIDI clock.
const byte PPQN = 24;                            // clock pulses per quarter note
const unsigned long CLOCK_TIMEOUT_MS = 600;      // clock considered absent after this

// Beat-lock subdivisions, in beats per unit (index 0 = slow .. 4 = fast).
// 1 bar, 1/2, 1/4, 1/8, 1/16.
const byte NUM_SUBDIV = 5;
const float SUBDIV_BEATS[NUM_SUBDIV] = {4.0, 2.0, 1.0, 0.5, 0.25};

// -------------------------------- CONTROLLER IO -----------------------------------
const byte SWITCH_PIN     = A0;   // mode switch  (D14)
const byte LED_PIN        = A1;   // NeoPixel data (D15)
const byte SEED_PIN       = A2;   // floating analog pin used to seed random()
const byte RE_CLK_PIN     = 3;    // encoder A / CLK (INT1)
const byte RE_DT_PIN      = 8;    // encoder B / DT
const byte RE_BUTTON_PIN  = 12;   // encoder push button

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
byte gBrightness[NUM_LIGHTS] = {0, 0, 0, 0};

// ---------------------------------- MIDI SETUP ------------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 15

// -------------------------------- NEOPIXEL SETUP ----------------------------------
const byte NUM_PIXELS = 8;
Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t COLOR_WARM;   // MANUAL mode color  (set in setup once ledStrip exists)
uint32_t COLOR_RED;    // ANIMATION mode color

// ---------------------------------- ANIMATIONS ------------------------------------
enum Anim {
  ANIM_COMET, ANIM_LARSON, ANIM_SINE, ANIM_BREATHE,
  ANIM_TWINKLE, ANIM_BUILD, ANIM_ALT, ANIM_STROBE, NUM_ANIMS
};

// -------------------------------- STATE / RUNTIME ---------------------------------
enum Mode { MODE_MANUAL, MODE_ANIM };
Mode gMode = MODE_MANUAL;

bool gStripDirty = true;

// MANUAL: target 0 = ALL, 1..4 = single light.
byte gSelected = 0;
bool gManualBrightActive = false;
unsigned long gManualBrightUntil = 0;

// Encoder button.
bool gButtonLast = false;
unsigned long gButtonLastTime = 0;

// Rotary encoder: polled quadrature state-table decoder (Ben Buxton style).
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

// ANIMATION mode.
byte gAnimSel = 0;                 // 0..NUM_ANIMS-1, highlighted/active animation
bool gAnimRunning = false;         // false = selection menu, true = running
int  gAnimSpeed[NUM_ANIMS];        // free-run speed per animation, 0..255
byte gAnimSubdiv[NUM_ANIMS];       // beat-lock subdivision per animation, 0..NUM_SUBDIV-1

// Animation render state.
unsigned long gStepAt = 0;         // last step time (stepwise anims)
unsigned long gLastRender = 0;     // last render time (continuous anims)
float gPhase = 0;                  // sine / breathe phase
int  gAnimPos = 0;                 // comet / larson position
int  gAnimDir = 1;                 // larson direction
byte gBuildStep = 0;
bool gAltState = false;
bool gStrobeOn = false;

// MIDI clock tracking.
byte gClockCount = 0;
unsigned long gBeatMs = 500;       // measured beat interval (default 120 BPM)
unsigned long gLastBeatAt = 0;
unsigned long gLastClockMs = 0;
bool gPrevClockActive = false;

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
  MIDI.setHandleContinue(handleStart);
  MIDI.setHandleStop(handleStop);

  pinMode(RE_CLK_PIN, INPUT_PULLUP);
  pinMode(RE_DT_PIN, INPUT_PULLUP);
  pinMode(RE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  randomSeed(analogRead(SEED_PIN));

  for (byte i = 0; i < NUM_ANIMS; i++) {
    gAnimSpeed[i] = 128;
    gAnimSubdiv[i] = 2;            // default 1/4 note (one step/cycle per beat)
  }

  ledStrip.begin();
  ledStrip.setBrightness(60);
  COLOR_WARM = ledStrip.Color(255, 130, 15);
  COLOR_RED  = ledStrip.Color(255, 0, 0);
  clearStrip();

  DimmableLight::setSyncPin(DIM_SYNC_PIN);
  DimmableLight::begin();

  gMode = readModeSwitch();
  onModeChange();
}

// ----------------------------------------------------------------------------------
// >x< LOOP >x<
// ----------------------------------------------------------------------------------
void loop() {
  Mode mode = readModeSwitch();
  if (mode != gMode) {
    gMode = mode;
    onModeChange();
  }

  MIDI.read();           // clock always; notes/CC act only in MANUAL (gated in handlers)
  handleEncoder();
  handleButton();

  // MANUAL: revert the brightness bar to the pixel-number display after a timeout.
  if (gMode == MODE_MANUAL && gManualBrightActive && millis() >= gManualBrightUntil) {
    gManualBrightActive = false;
    gStripDirty = true;
  }

  // ANIMATION: refresh the parameter readout when the clock appears/disappears.
  bool ca = clockActive();
  if (gMode == MODE_ANIM && gAnimRunning && ca != gPrevClockActive) {
    gStripDirty = true;
  }
  gPrevClockActive = ca;

  if (gMode == MODE_ANIM && gAnimRunning) {
    updateAnim();
  }

  if (gStripDirty) {
    drawStrip();
    gStripDirty = false;
  }
}

Mode readModeSwitch() {
  return (digitalRead(SWITCH_PIN) == ANIM_MODE_LEVEL) ? MODE_ANIM : MODE_MANUAL;
}

void onModeChange() {
  gEncoderState = R_START;
  if (gMode == MODE_MANUAL) {
    gSelected = 0;
    gManualBrightActive = false;
  } else {
    gAnimRunning = false;          // start in the selection menu
  }
  gStripDirty = true;
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
  for (byte i = 0; i < NUM_LIGHTS; i++) setLight(i, value);
}

// ----------------------------------------------------------------------------------
// ROTARY ENCODER
// ----------------------------------------------------------------------------------
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
  bool right = (dir == DIR_CCW);   // DIR_CW is a left turn on this wiring

  if (gMode == MODE_MANUAL) {
    int delta = right ? ENCODER_STEP : -ENCODER_STEP;
    if (gSelected == 0) {
      for (byte i = 0; i < NUM_LIGHTS; i++) setLight(i, gBrightness[i] + delta);
    } else {
      setLight(gSelected - 1, gBrightness[gSelected - 1] + delta);
    }
    gManualBrightActive = true;                 // show the brightness bar
    gManualBrightUntil = millis() + MANUAL_BRIGHT_SHOW_MS;
    gStripDirty = true;
  } else if (!gAnimRunning) {                   // ANIMATION menu: move the cursor
    gAnimSel = (gAnimSel + (right ? 1 : NUM_ANIMS - 1)) % NUM_ANIMS;
    gStripDirty = true;
  } else if (clockActive()) {                   // running, clocked: pick subdivision
    gAnimSubdiv[gAnimSel] = constrain(gAnimSubdiv[gAnimSel] + (right ? 1 : -1), 0, NUM_SUBDIV - 1);
    gStripDirty = true;
  } else {                                      // running, free: set speed
    gAnimSpeed[gAnimSel] = constrain(gAnimSpeed[gAnimSel] + (right ? ENCODER_STEP : -ENCODER_STEP), 0, 255);
    gStripDirty = true;
  }
}

void handleButton() {
  bool pressed = (digitalRead(RE_BUTTON_PIN) == LOW);
  if (pressed && !gButtonLast && (millis() - gButtonLastTime > BUTTON_DEBOUNCE_MS)) {
    gButtonLastTime = millis();
    if (gMode == MODE_MANUAL) {
      gSelected = (gSelected + 1) % (NUM_LIGHTS + 1);
      gManualBrightActive = false;
      gStripDirty = true;
    } else if (!gAnimRunning) {
      gAnimRunning = true;                       // enter the selected animation
      resetAnimState();
      gStripDirty = true;
    } else {
      gAnimRunning = false;                      // exit back to the menu
      setAllLights(0);
      gStripDirty = true;
    }
  }
  gButtonLast = pressed;
}

// ----------------------------------------------------------------------------------
// MIDI CLOCK
// ----------------------------------------------------------------------------------
bool clockActive() {
  return gLastClockMs != 0 && (millis() - gLastClockMs < CLOCK_TIMEOUT_MS);
}

void handleClock() {
  gLastClockMs = millis();
  if (++gClockCount >= PPQN) {
    gClockCount = 0;
    unsigned long now = millis();
    if (gLastBeatAt != 0) {
      unsigned long d = now - gLastBeatAt;
      if (d > 50 && d < 3000) gBeatMs = d;       // ~20-1200 BPM
    }
    gLastBeatAt = now;
  }
}

void handleStart() {
  gClockCount = 0;
  gLastBeatAt = 0;
}

void handleStop() {
  gClockCount = 0;
}

// ----------------------------------------------------------------------------------
// MIDI NOTE / CC HANDLERS (MANUAL mode only)
// ----------------------------------------------------------------------------------
void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (gMode != MODE_MANUAL) return;
  if (velocity == 0) { handleNoteOff(channel, pitch, velocity); return; }
  for (byte i = 0; i < NUM_LIGHTS; i++) {
    if (pitch == NOTE_LIGHT[i]) { setLight(i, map(velocity, 1, 127, NOTE_MIN_BRIGHT, 255)); break; }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  if (gMode != MODE_MANUAL) return;
  for (byte i = 0; i < NUM_LIGHTS; i++) {
    if (pitch == NOTE_LIGHT[i]) { setLight(i, 0); break; }
  }
}

void handleControlChange(byte channel, byte number, byte value) {
  if (gMode != MODE_MANUAL) return;
  for (byte i = 0; i < NUM_LIGHTS; i++) {
    if (number == CC_LIGHT[i]) { setLight(i, map(value, 0, 127, 0, 255)); return; }
  }
  if (number == CC_ALL_BRIGHT) setAllLights(map(value, 0, 127, 0, 255));
}

// ----------------------------------------------------------------------------------
// ANIMATIONS
// ----------------------------------------------------------------------------------
bool isContinuous(byte a) {
  return a == ANIM_SINE || a == ANIM_BREATHE;
}

void resetAnimState() {
  gPhase = 0;
  gLastRender = millis();
  gStepAt = millis();
  gAnimPos = 0;
  gAnimDir = 1;
  gBuildStep = 0;
  gAltState = false;
  gStrobeOn = false;
  setAllLights(0);
  if (!isContinuous(gAnimSel)) renderStep(gAnimSel);   // show the first frame now
}

// One "unit" in ms: a beat subdivision when clocked, else the free-run speed.
unsigned long animUnitMs() {
  if (clockActive()) {
    long ms = (long)(SUBDIV_BEATS[gAnimSubdiv[gAnimSel]] * gBeatMs);
    return max(20L, ms);
  }
  return map(gAnimSpeed[gAnimSel], 0, 255, UNIT_MS_MAX, UNIT_MS_MIN);  // right = faster
}

void updateAnim() {
  unsigned long unit = animUnitMs();
  byte a = gAnimSel;
  if (isContinuous(a)) {
    unsigned long now = millis();
    gPhase += TWO_PI * (float)(now - gLastRender) / (float)unit;
    gLastRender = now;
    while (gPhase > TWO_PI) gPhase -= TWO_PI;
    renderContinuous(a);
  } else if (millis() - gStepAt >= unit) {
    gStepAt = millis();
    renderStep(a);
  }
}

void decayAll() {
  for (byte i = 0; i < NUM_LIGHTS; i++) setLight(i, gBrightness[i] - (gBrightness[i] >> TAIL_SHIFT));
}

void renderStep(byte a) {
  switch (a) {
    case ANIM_COMET:
      decayAll();
      gAnimPos = (gAnimPos + 1) % NUM_LIGHTS;
      setLight(gAnimPos, 255);
      break;
    case ANIM_LARSON:
      decayAll();
      gAnimPos += gAnimDir;
      if (gAnimPos >= NUM_LIGHTS - 1) { gAnimPos = NUM_LIGHTS - 1; gAnimDir = -1; }
      else if (gAnimPos <= 0)        { gAnimPos = 0;              gAnimDir =  1; }
      setLight(gAnimPos, 255);
      break;
    case ANIM_TWINKLE:
      decayAll();
      setLight(random(NUM_LIGHTS), 255);
      break;
    case ANIM_BUILD:
      gBuildStep = (gBuildStep + 1) % (NUM_LIGHTS + 1);     // 0 = blackout
      for (byte i = 0; i < NUM_LIGHTS; i++) setLight(i, i < gBuildStep ? 255 : 0);
      break;
    case ANIM_ALT:
      gAltState = !gAltState;
      setLight(0, gAltState ? 255 : 0);
      setLight(2, gAltState ? 255 : 0);
      setLight(1, gAltState ? 0 : 255);
      setLight(3, gAltState ? 0 : 255);
      break;
    case ANIM_STROBE:
      gStrobeOn = !gStrobeOn;
      setAllLights(gStrobeOn ? 255 : 0);
      break;
  }
}

void renderContinuous(byte a) {
  if (a == ANIM_SINE) {
    for (byte i = 0; i < NUM_LIGHTS; i++) {
      float s = sin(gPhase + i * (TWO_PI / NUM_LIGHTS));
      setLight(i, (int)((s * 0.5f + 0.5f) * 255));
    }
  } else {  // ANIM_BREATHE
    setAllLights((int)((sin(gPhase) * 0.5f + 0.5f) * 255));
  }
}

// ----------------------------------------------------------------------------------
// NEOPIXEL STRIP
// ----------------------------------------------------------------------------------
void clearStrip() {
  for (byte i = 0; i < NUM_PIXELS; i++) ledStrip.setPixelColor(i, 0);
  ledStrip.show();
}

void fillPixels(uint32_t color, byte litCount) {
  for (byte i = 0; i < NUM_PIXELS; i++) ledStrip.setPixelColor(i, i < litCount ? color : 0);
  ledStrip.show();
}

void lightOnePixel(byte index, uint32_t color) {
  for (byte i = 0; i < NUM_PIXELS; i++) ledStrip.setPixelColor(i, i == index ? color : 0);
  ledStrip.show();
}

void drawStrip() {
  if (gMode == MODE_MANUAL) {
    if (gManualBrightActive) {                      // brightness bar (warm yellow)
      byte b;
      if (gSelected == 0) {
        int sum = 0;
        for (byte i = 0; i < NUM_LIGHTS; i++) sum += gBrightness[i];
        b = sum / NUM_LIGHTS;
      } else {
        b = gBrightness[gSelected - 1];
      }
      fillPixels(COLOR_WARM, map(b, 0, 255, 0, NUM_PIXELS));
    } else if (gSelected == 0) {                     // ALL -> every pixel
      fillPixels(COLOR_WARM, NUM_PIXELS);
    } else {                                         // Light N -> the Nth pixel
      lightOnePixel(gSelected - 1, COLOR_WARM);
    }
  } else {                                           // ANIMATION (red)
    if (!gAnimRunning) {
      lightOnePixel(gAnimSel, COLOR_RED);            // selection cursor
    } else if (clockActive()) {
      fillPixels(COLOR_RED, gAnimSubdiv[gAnimSel] + 1);                 // subdivision 1..5
    } else {
      fillPixels(COLOR_RED, map(gAnimSpeed[gAnimSel], 0, 255, 1, NUM_PIXELS));  // speed
    }
  }
}
