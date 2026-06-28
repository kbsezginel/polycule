// ----------------------------------------------------------------------------------
// POLYCULE | LUNT | MIDI-enabled light controller
// ----------------------------------------------------------------------------------
// Arduino Nano + RobotDyn 4-channel AC dimmer + rotary encoder + 8px NeoPixel strip.
//
// The SWITCH chooses one of two modes:
//
//   * MANUAL (strip = warm yellow; no clock indicators shown here)
//       - Encoder press : cycle ALL -> 1 -> 2 -> 3 -> 4 -> STRIP -> TEMPO -> SUBDIV.
//       - Encoder turn  : set the target's brightness (MIDI notes/CC also set bulbs).
//         STRIP  = NeoPixel indicator brightness (applies to all modes).
//         TEMPO  = toggle the tempo/clock indicator on/off.
//         SUBDIV = toggle the subdivision indicator on/off.
//       - Strip: ALL = all pixels, Light N = the Nth pixel. Turning briefly shows a
//         brightness bar, then reverts to the pixel-number display.
//
//   * ANIMATION (strip = red)
//       - Selection menu: turn to move the cursor over the 8 animations (one pixel each),
//         press to enter, press again to exit back to the menu.
//       - In an animation: encoder sets its parameter (shown on the strip). A short press
//         re-syncs the animation to that moment (tap on the beat); a long hold exits to the
//         menu. MIDI notes/CC are ignored. If a MIDI clock is present the animation
//         beat-locks and the encoder picks the subdivision (1 bar / 1/2 / 1/4 / 1/8 / 1/16).
//         The right two pixels show the tempo (blue=no clock / purple=beat) and subdivision.
//       - Animations: comet, larson, sine, drift, twinkle, build, waves, 6/8 pulse.
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
const unsigned long LONG_PRESS_MS = 600;    // hold this long inside an animation to exit
const byte MANUAL_RESET_BRIGHTNESS = 128;   // all bulbs reset to this on entering MANUAL
const byte STRIP_BRIGHTNESS_DEFAULT = 60;   // NeoPixel indicator brightness at boot, 0-255
const byte STRIP_BRIGHTNESS_MIN = 5;        // floor so the indicator never goes fully dark

// MIDI note -> light mapping (MANUAL mode). Note ON sets brightness from velocity,
// Note OFF turns the light fully off.
const byte NOTE_LIGHT[4]   = {60, 61, 62, 63};   // C4, C#4, D4, D#4
const byte NOTE_MIN_BRIGHT = 40;                 // softest visible "on" level
// MIDI CC -> light brightness mapping (MANUAL mode).
const byte CC_LIGHT[4]     = {22, 23, 24, 25};
const byte CC_ALL_BRIGHT   = 27;                 // brightness of all lights at once

// Animation timing. Free-run speed maps (linearly in BPM) to a "unit" = one step
// (stepwise anims) or one cycle (continuous anims), treating one unit as a beat.
const int  ANIM_BPM_MIN = 20;                    // encoder fully left  (slowest, ~3s/unit)
const int  ANIM_BPM_MAX = 250;                   // encoder fully right (fastest)
const byte TAIL_SHIFT = 2;                       // comet/larson/twinkle fade: b -= b >> this

// Sine LFO subdivision: the sine completes one cycle every SINE_SUBDIV beats, so its rate
// is the BPM divided by this. Use {1, 2, 3, 4, 5, 6, 7, 9} for 1, 1/2, 1/3 ... 1/9.
const byte SINE_SUBDIV = 4;                      // default 1/4 (one cycle every 4 beats)
// Sine LFO brightness range, as a percent of full. The sine swings between these.
const byte SINE_MIN_PCT = 2;                     // trough brightness (% of 255)
const byte SINE_MAX_PCT = 66;                    // peak brightness  (% of 255)

// Waves animation. A slow, smooth sine swell. WAVE_SPREAD is the phase offset between
// adjacent bulbs (0 = all in phase, PI/2 = a wave rolling across the row, PI = neighbours
// opposite). WAVE_FLOOR keeps a minimum glow so the bulbs never go fully dark (calmer).
const float WAVE_SPREAD = 1.5708f;                 // ~90 deg between adjacent bulbs (0 = in phase)
const byte  WAVE_FLOOR  = 40;                      // 0-255 minimum brightness (higher = gentler swell)
const int   WAVES_SPEED = 23;                      // gAnimSpeed default -> ~40 BPM (slow / calming)

// Drift animation. Each bulb is its own oscillator running at a slightly different rate,
// so they wander in and out of phase independently (no travelling-wave motion). Per-bulb
// rate = 1 + DRIFT_SPREAD * (DRIFT_MULT[i] - 1); DRIFT_SPREAD 0 = all in sync (a plain
// breathe), 1 = fully independent. Bulb 0 stays at the base rate (the encoder's BPM).
const float DRIFT_MULT[4] = {1.00f, 1.18f, 0.86f, 1.34f};          // per-bulb rate at full spread
const float DRIFT_SPREAD = 0.5f;                   // 0 = synced, 1 = fully independent
const int   DRIFT_SPEED  = 12;                     // gAnimSpeed default -> ~30 BPM (slow / calming)

// 6/8 pulse animation. Each beat the active pair flashes at the onset and fades out;
// the peak steps down across the three beats of each group (strong, medium, weak).
const float PULSE68_PEAK[3] = {1.0f, 0.6f, 0.3f};  // per-beat peak within a group of three
const float PULSE68_DECAY   = 3.5f;                // within-beat flash decay (higher = snappier)
const int   PULSE68_SPEED   = 111;                 // gAnimSpeed default -> ~120 BPM (maps 0-255 to 20-250)

// Strip feedback timing.
const unsigned long MANUAL_BRIGHT_SHOW_MS = 1200;  // brightness bar shown after a turn

// MIDI clock.
const byte PPQN = 24;                            // clock pulses per quarter note
const unsigned long CLOCK_TIMEOUT_MS = 600;      // clock considered absent after this
const unsigned long CLOCK_BLINK_MS = 90;         // how long the beat flash stays lit

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
uint32_t COLOR_WARM;            // MANUAL mode color  (set in setup once ledStrip exists)
uint32_t COLOR_RED;             // ANIMATION mode color
uint32_t COLOR_TEMPO_BLUE;      // tempo light, no clock (steady)
uint32_t COLOR_TEMPO_PURPLE;    // tempo light, clock present (flashes on the beat)
uint32_t COLOR_SUBDIV_ORANGE;   // subdivision light (clock present, flashes per subdivision)
const byte TEMPO_PIXEL  = NUM_PIXELS - 1;   // far-right: tempo / clock indicator
const byte SUBDIV_PIXEL = NUM_PIXELS - 2;   // second from right: subdivision indicator

// ---------------------------------- ANIMATIONS ------------------------------------
enum Anim {
  ANIM_COMET, ANIM_LARSON, ANIM_SINE, ANIM_DRIFT,
  ANIM_TWINKLE, ANIM_BUILD, ANIM_WAVES, ANIM_PULSE68, NUM_ANIMS
};

// -------------------------------- STATE / RUNTIME ---------------------------------
enum Mode { MODE_MANUAL, MODE_ANIM };
Mode gMode = MODE_MANUAL;

bool gStripDirty = true;

// MANUAL targets, cycled by the encoder button:
//   0 = ALL, 1..4 = single light, STRIP = NeoPixel brightness,
//   TEMPO_LIGHT / SUBDIV_LIGHT = toggle the clock indicators on/off.
const byte STRIP_TARGET       = NUM_LIGHTS + 1;   // = 5
const byte TEMPO_LIGHT_TARGET = NUM_LIGHTS + 2;   // = 6
const byte SUBDIV_LIGHT_TARGET = NUM_LIGHTS + 3;  // = 7
const byte NUM_MANUAL_TARGETS = NUM_LIGHTS + 4;   // = 8
byte gSelected = 0;
bool gManualBrightActive = false;
unsigned long gManualBrightUntil = 0;
byte gStripBrightness = STRIP_BRIGHTNESS_DEFAULT;   // NeoPixel brightness, applies to all modes
bool gTempoLightOn = true;     // show the tempo / clock indicator (far-right pixel)
bool gSubdivLightOn = true;    // show the subdivision indicator (second from right)

// Encoder button (with press-down / long-hold detection).
bool gBtnDown = false;
unsigned long gBtnDownAt = 0;
unsigned long gBtnUpAt = 0;
bool gBtnLongFired = false;

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
float gPhase = 0;                  // sine / waves / pulse phase
float gDriftPhase[4] = {0, 0, 0, 0};  // drift: per-bulb independent phase
int  gAnimPos = 0;                 // comet / larson position
int  gAnimDir = 1;                 // larson direction
byte gBuildStep = 0;
byte gMeterBeat = 0;               // 6/8 pulse: current beat of the 6-beat measure (0..5)

// MIDI clock tracking.
byte gClockCount = 0;
unsigned long gBeatMs = 500;       // measured beat interval (default 120 BPM)
unsigned long gLastBeatAt = 0;
unsigned long gLastClockMs = 0;
unsigned long gBeatPulseAt = 0;    // time of the last beat (drives the tempo flash)
unsigned long gUnitPulseAt = 0;    // time of the last animation unit (drives the subdiv flash)
bool gPrevClockActive = false;
bool gTempoBlinkOn = false;
bool gSubdivBlinkOn = false;

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
  gAnimSpeed[ANIM_DRIFT]   = DRIFT_SPEED;     // drift defaults to a slow ~30 BPM
  gAnimSpeed[ANIM_WAVES]   = WAVES_SPEED;     // waves default to a slow ~40 BPM
  gAnimSpeed[ANIM_PULSE68] = PULSE68_SPEED;   // 6/8 pulse defaults to ~120 BPM

  ledStrip.begin();
  ledStrip.setBrightness(gStripBrightness);
  COLOR_WARM = ledStrip.Color(255, 130, 15);
  COLOR_RED  = ledStrip.Color(255, 0, 0);
  COLOR_TEMPO_BLUE    = ledStrip.Color(0, 0, 255);     // no clock
  COLOR_TEMPO_PURPLE  = ledStrip.Color(160, 0, 255);   // clock present
  COLOR_SUBDIV_ORANGE = ledStrip.Color(255, 90, 0);
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

  // Animation-mode indicators: redraw when the clock appears/disappears or a flash toggles.
  // Tempo light blinks on the beat when clocked, else at the free-run unit (selected BPM).
  bool ca = clockActive();
  unsigned long tempoPulseAt = ca ? gBeatPulseAt : gUnitPulseAt;
  bool tBlink = (millis() - tempoPulseAt < CLOCK_BLINK_MS);
  bool sBlink = ca && (millis() - gUnitPulseAt < CLOCK_BLINK_MS);
  if (gMode == MODE_ANIM && gAnimRunning &&
      (ca != gPrevClockActive || tBlink != gTempoBlinkOn || sBlink != gSubdivBlinkOn)) {
    gStripDirty = true;
  }
  gPrevClockActive = ca;
  gTempoBlinkOn = tBlink;
  gSubdivBlinkOn = sBlink;

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
    setAllLights(MANUAL_RESET_BRIGHTNESS);   // clean slate (no bulbs stuck from an anim)
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
    if (gSelected == STRIP_TARGET) {            // set the NeoPixel indicator brightness
      gStripBrightness = constrain(gStripBrightness + delta, STRIP_BRIGHTNESS_MIN, 255);
      ledStrip.setBrightness(gStripBrightness);
      gStripDirty = true;
    } else if (gSelected == TEMPO_LIGHT_TARGET) {   // toggle the tempo indicator
      gTempoLightOn = right;                        // right = on, left = off
      gStripDirty = true;
    } else if (gSelected == SUBDIV_LIGHT_TARGET) {  // toggle the subdivision indicator
      gSubdivLightOn = right;
      gStripDirty = true;
    } else {
      if (gSelected == 0) {
        for (byte i = 0; i < NUM_LIGHTS; i++) setLight(i, gBrightness[i] + delta);
      } else {
        setLight(gSelected - 1, gBrightness[gSelected - 1] + delta);
      }
      gManualBrightActive = true;               // show the brightness bar
      gManualBrightUntil = millis() + MANUAL_BRIGHT_SHOW_MS;
      gStripDirty = true;
    }
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
  unsigned long now = millis();

  if (pressed && !gBtnDown && (now - gBtnUpAt > BUTTON_DEBOUNCE_MS)) {
    // --- press-down edge ---
    gBtnDown = true;
    gBtnDownAt = now;
    gBtnLongFired = false;
    if (gMode == MODE_MANUAL) {
      gSelected = (gSelected + 1) % NUM_MANUAL_TARGETS;   // ALL, 1-4, STRIP, TEMPO, SUBDIV
      gManualBrightActive = false;
      gStripDirty = true;
    } else if (!gAnimRunning) {
      gAnimRunning = true;             // enter the selected animation
      resetAnimState();
      gStripDirty = true;
      gBtnLongFired = true;            // the entering press isn't a re-sync/exit
    } else {
      resetAnimState();               // short press = re-sync, anchored to this moment
      gStripDirty = true;
    }
  } else if (pressed && gBtnDown && !gBtnLongFired &&
             gMode == MODE_ANIM && gAnimRunning && (now - gBtnDownAt >= LONG_PRESS_MS)) {
    // --- long hold inside an animation -> exit to the menu ---
    gAnimRunning = false;
    setAllLights(0);
    gStripDirty = true;
    gBtnLongFired = true;
  } else if (!pressed && gBtnDown) {
    // --- release ---
    gBtnDown = false;
    gBtnUpAt = now;
  }
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
    gBeatPulseAt = now;                          // flash the clock indicator on the beat
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
  return a == ANIM_SINE || a == ANIM_DRIFT || a == ANIM_WAVES || a == ANIM_PULSE68;
}

void resetAnimState() {
  gPhase = 0;
  gLastRender = millis();
  gStepAt = millis();
  gUnitPulseAt = millis();
  gAnimPos = 0;
  gAnimDir = 1;
  gBuildStep = 0;
  gMeterBeat = 0;
  for (byte i = 0; i < NUM_LIGHTS; i++) gDriftPhase[i] = 0;   // drift bulbs start in sync
  setAllLights(0);
  if (!isContinuous(gAnimSel)) renderStep(gAnimSel);   // show the first frame now
}

// One "unit" in ms: a beat subdivision when clocked, else the free-run speed.
unsigned long animUnitMs() {
  if (clockActive()) {
    long ms = (long)(SUBDIV_BEATS[gAnimSubdiv[gAnimSel]] * gBeatMs);
    return max(20L, ms);
  }
  int bpm = map(gAnimSpeed[gAnimSel], 0, 255, ANIM_BPM_MIN, ANIM_BPM_MAX);  // right = faster
  unsigned long unit = 60000UL / bpm;                                       // one unit = one beat
  if (gAnimSel == ANIM_SINE) unit *= SINE_SUBDIV;                           // sine: one cycle every N beats
  return unit;
}

void updateAnim() {
  unsigned long unit = animUnitMs();
  byte a = gAnimSel;
  if (isContinuous(a)) {
    unsigned long now = millis();
    float inc = TWO_PI * (float)(now - gLastRender) / (float)unit;      // phase advanced this frame
    gLastRender = now;
    gPhase += inc;
    while (gPhase > TWO_PI) {                                           // one pulse per cycle
      gPhase -= TWO_PI;
      gUnitPulseAt = now;
      if (a == ANIM_PULSE68) gMeterBeat = (gMeterBeat + 1) % 6;         // advance the 6/8 beat
    }
    if (a == ANIM_DRIFT) {                                              // each bulb at its own rate
      for (byte i = 0; i < NUM_LIGHTS; i++) {
        gDriftPhase[i] += inc * (1.0f + DRIFT_SPREAD * (DRIFT_MULT[i] - 1.0f));
        while (gDriftPhase[i] > TWO_PI) gDriftPhase[i] -= TWO_PI;
      }
    }
    renderContinuous(a);
  } else if (millis() - gStepAt >= unit) {
    gStepAt = millis();
    gUnitPulseAt = millis();                                           // one pulse per step
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
      setLight(gAnimPos, 255);                  // light current, then advance
      gAnimPos = (gAnimPos + 1) % NUM_LIGHTS;
      break;
    case ANIM_LARSON:
      decayAll();
      setLight(gAnimPos, 255);                  // light current, then advance + bounce
      gAnimPos += gAnimDir;
      if (gAnimPos >= NUM_LIGHTS - 1) { gAnimPos = NUM_LIGHTS - 1; gAnimDir = -1; }
      else if (gAnimPos <= 0)        { gAnimPos = 0;              gAnimDir =  1; }
      break;
    case ANIM_TWINKLE:
      decayAll();
      setLight(random(NUM_LIGHTS), 255);
      break;
    case ANIM_BUILD:
      gBuildStep = (gBuildStep + 1) % (NUM_LIGHTS + 1);     // 0 = blackout
      for (byte i = 0; i < NUM_LIGHTS; i++) setLight(i, i < gBuildStep ? 255 : 0);
      break;
  }
}

void renderContinuous(byte a) {
  if (a == ANIM_SINE) {          // simple sine LFO: all bulbs together, between min and max
    float s = sin(gPhase) * 0.5f + 0.5f;                 // 0..1
    int lo = (int)SINE_MIN_PCT * 255 / 100;
    int hi = (int)SINE_MAX_PCT * 255 / 100;
    setAllLights(lo + (int)((hi - lo) * s));
  } else if (a == ANIM_DRIFT) {  // each bulb on its own phase (advanced per-rate in updateAnim)
    for (byte i = 0; i < NUM_LIGHTS; i++)
      setLight(i, (int)((sin(gDriftPhase[i]) * 0.5f + 0.5f) * 255));
  } else if (a == ANIM_WAVES) {  // smooth sine swell, tunable phase spread + a minimum glow
    for (byte i = 0; i < NUM_LIGHTS; i++) {
      float s = 0.5f + 0.5f * sin(gPhase + i * WAVE_SPREAD);
      setLight(i, WAVE_FLOOR + (int)((255 - WAVE_FLOOR) * s));
    }
  } else {  // ANIM_PULSE68 - 6/8 pulse: flash on each beat, fade out, peak steps down per group
    float frac = gPhase / TWO_PI;                         // 0..1 through the current beat
    float env  = expf(-frac * PULSE68_DECAY);             // flash at the onset, then fade
    int   v    = (int)(PULSE68_PEAK[gMeterBeat % 3] * env * 255);
    bool  firstGroup = (gMeterBeat < 3);                  // beats 0-2 = lights 1&2, 3-5 = lights 3&4
    setLight(0, firstGroup ? v : 0);
    setLight(1, firstGroup ? v : 0);
    setLight(2, firstGroup ? 0 : v);
    setLight(3, firstGroup ? 0 : v);
  }
}

// ----------------------------------------------------------------------------------
// NEOPIXEL STRIP
// ----------------------------------------------------------------------------------
// Logical pixel 0 is the far-left LED. The strip's data-in end is physically on the
// right, so map logical -> physical in reverse. The buffer helpers don't show(); the
// indicator pixels are at most NUM_PIXELS-2 from the left, so drawStrip() composites
// the content and then overlays the indicators with a single show().
void setLogicalPixel(byte index, uint32_t color) {
  ledStrip.setPixelColor(NUM_PIXELS - 1 - index, color);
}

void clearBuffer() {
  for (byte i = 0; i < NUM_PIXELS; i++) ledStrip.setPixelColor(i, 0);
}

void clearStrip() {
  clearBuffer();
  ledStrip.show();
}

void fillPixels(uint32_t color, byte litCount) {
  for (byte i = 0; i < NUM_PIXELS; i++)
    ledStrip.setPixelColor(i, i >= (NUM_PIXELS - litCount) ? color : 0);
}

void lightOnePixel(byte index, uint32_t color) {
  clearBuffer();
  setLogicalPixel(index, color);
}

void drawManualContent() {
  if (gSelected == STRIP_TARGET) {                   // strip brightness bar (dims live)
    fillPixels(COLOR_WARM, map(gStripBrightness, 0, 255, 1, NUM_PIXELS));
  } else if (gSelected == TEMPO_LIGHT_TARGET) {      // tempo-light on/off preview
    if (gTempoLightOn) lightOnePixel(TEMPO_PIXEL, COLOR_TEMPO_BLUE); else clearBuffer();
  } else if (gSelected == SUBDIV_LIGHT_TARGET) {     // subdivision-light on/off preview
    if (gSubdivLightOn) lightOnePixel(SUBDIV_PIXEL, COLOR_SUBDIV_ORANGE); else clearBuffer();
  } else if (gManualBrightActive) {                  // bulb brightness bar
    byte b;
    if (gSelected == 0) {
      int sum = 0;
      for (byte i = 0; i < NUM_LIGHTS; i++) sum += gBrightness[i];
      b = sum / NUM_LIGHTS;
    } else {
      b = gBrightness[gSelected - 1];
    }
    fillPixels(COLOR_WARM, map(b, 0, 255, 0, NUM_PIXELS));
  } else if (gSelected == 0) {                       // ALL -> every pixel
    fillPixels(COLOR_WARM, NUM_PIXELS);
  } else {                                           // Light N -> the Nth pixel
    lightOnePixel(gSelected - 1, COLOR_WARM);
  }
}

// Overlay the clock indicators on the right two pixels (ANIMATION, running only).
void drawAnimIndicators() {
  // Tempo light flashes each unit: blue at the free-run speed, purple on the MIDI beat.
  if (gTempoLightOn && gTempoBlinkOn) {
    setLogicalPixel(TEMPO_PIXEL, clockActive() ? COLOR_TEMPO_PURPLE : COLOR_TEMPO_BLUE);
  }
  if (gSubdivLightOn && clockActive() && gSubdivBlinkOn) {
    setLogicalPixel(SUBDIV_PIXEL, COLOR_SUBDIV_ORANGE);              // orange flash per subdivision
  }
}

void drawStrip() {
  if (gMode == MODE_MANUAL) {                         // no MIDI indicators in MANUAL
    drawManualContent();
  } else if (!gAnimRunning) {                         // ANIMATION selection menu
    lightOnePixel(gAnimSel, COLOR_RED);
  } else {                                            // ANIMATION running
    byte cap = NUM_PIXELS - 2;                        // leave the right two pixels for indicators
    byte n = clockActive() ? (gAnimSubdiv[gAnimSel] + 1)
                           : map(gAnimSpeed[gAnimSel], 0, 255, 1, cap);
    if (n > cap) n = cap;
    fillPixels(COLOR_RED, n);
    drawAnimIndicators();
  }
  ledStrip.show();
}
