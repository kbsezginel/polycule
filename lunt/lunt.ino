// ----------------------------------------------------------------------------------
// POLYCULE | LUNT | MIDI-enabled light controller
// ----------------------------------------------------------------------------------
// Arduino Nano + RobotDyn 4-channel AC dimmer + rotary encoder + 8px NeoPixel strip
// + microphone amp module.
//
// The SWITCH chooses one of two top-level modes:
//
//   * MIDI+MANUAL -> MIDI and the encoder are both live.
//       - Encoder press : cycle the target ALL -> 1 -> 2 -> 3 -> 4 -> animations.
//       - Encoder turn  : on ALL/1-4, set brightness; on an animation, set its speed.
//       - Incoming MIDI : notes turn bulbs on/off, CC sets brightness, clock can pulse.
//       - Self-running animations (no input): comet, sine sweep, breathe, larson,
//         twinkle. Strip shows the target/brightness or the animation number.
//
//   * AUDIO -> react to a microphone; MIDI is ignored.
//       - Encoder press : select the animation (number shown on the strip).
//       - Encoder turn  : adjust the current animation's parameter (shown briefly).
//       Animations: 1) amplitude->brightness  2) amplitude->brightness w/ LPF
//                    3) spectrum split  4) beat pulse  5) breathing  6) sidechain/duck
// ----------------------------------------------------------------------------------
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>
#include "dimmable_light.h"

// ================================ CONFIG / KNOBS ==================================
// Switch level that selects AUDIO mode. The switch is INPUT_PULLUP, so it idles HIGH
// (= MIDI+MANUAL) and reads LOW when closed to GND (= AUDIO). Flip if reversed.
#define AUDIO_MODE_LEVEL   LOW

// Encoder feel (per detent / click of the knob).
const int  ENCODER_STEP      = 8;   // brightness (or anim speed) change in MIDI+MANUAL
const int  AUDIO_PARAM_STEP  = 8;   // animation-parameter change in AUDIO mode

// MIDI+MANUAL self-running animations (no input needed). Frame rate is set by the
// per-animation speed; the encoder adjusts it (right = faster).
const int   MANUAL_FRAME_MIN_MS = 12;    // frame time at full speed
const int   MANUAL_FRAME_MAX_MS = 180;   // frame time at slowest
const float MANUAL_PHASE_STEP   = 0.20;  // sine/breathe radians advanced per frame
const byte  MANUAL_TAIL_SHIFT   = 2;     // comet/larson/twinkle fade: b -= b >> this

// MIDI note -> light mapping (one note per light). Note ON sets brightness from
// velocity, Note OFF turns the light fully off.
const byte NOTE_LIGHT[4]    = {60, 61, 62, 63};   // C4, C#4, D4, D#4
const byte NOTE_MIN_BRIGHT  = 40;                 // softest visible "on" level

// MIDI CC -> light brightness mapping, plus a control CC.
const byte CC_LIGHT[4]      = {22, 23, 24, 25};
const byte CC_ALL_BRIGHT    = 27;                 // brightness of all lights at once
const byte CC_CLOCK_ANIM    = 26;                 // >=64 pulses bulbs on the MIDI beat

// Set true to let the MIDI clock pulse the bulbs on every beat by default.
bool gClockAnim = false;

// --- Audio (mic amp module on AUDIO_PIN, output biased to ~2.5V; e.g. MAX4466) -----
const byte AUDIO_PIN          = A2;
const unsigned long AUDIO_WINDOW_MS = 25;   // loudness is measured over this window
const int  AUDIO_PP_MIN       = 20;         // peak-to-peak below this = silence (off)
const int  AUDIO_PP_FULL_MAX  = 800;        // pp for full brightness at min sensitivity
const int  AUDIO_PP_FULL_MIN  = 60;         // pp for full brightness at max sensitivity
const int  AUDIO_PP_FULL_LPF  = 250;        // full-scale pp for the LPF animation
const byte AUDIO_RELEASE      = 6;          // how fast brightness falls per window (VU)
const unsigned long AUDIO_PARAM_SHOW_MS = 1200;  // how long the param bar stays up
// One-pole filter speeds (alpha/256 per sample). These set the spectrum-split
// crossovers and the beat-detector's bass band; tune if the bands feel off.
const int  DC_ALPHA           = 2;          // DC blocker (removes the ~2.5V bias)
const int  SPLIT_ALPHA1       = 8;          // bass | low-mid crossover
const int  SPLIT_ALPHA2       = 40;         // low-mid | high-mid crossover
const int  SPLIT_ALPHA3       = 140;        // high-mid | treble crossover
const int  AUDIO_PP_FULL_SPLIT_MAX = 400;   // per-band full-scale at min sensitivity
const int  AUDIO_PP_FULL_SPLIT_MIN = 60;    // per-band full-scale at max sensitivity
const int  BEAT_BASS_ALPHA    = 30;         // bass band used for beat detection
const int  BEAT_AVG_ALPHA     = 24;         // running-average adaptation per window
const int  BEAT_PP_MIN        = 25;         // ignore beats quieter than this
const byte BEAT_DECAY         = 28;         // beat pulse fade per window
const int  AUDIO_PP_FULL_BREATH = 400;      // full-scale loudness for breathing
const int  AUDIO_PP_FULL_DUCK   = 400;      // full-scale loudness for sidechain duck
const byte SIDE_RELEASE       = 8;          // how fast sidechain recovers per window

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
enum TopMode { MODE_MIDI_MANUAL, MODE_AUDIO };
TopMode gMode = MODE_MIDI_MANUAL;

bool gStripDirty = true;           // MIDI+MANUAL strip needs redraw

// MIDI+MANUAL selection: 0 = ALL, 1..4 = single light (NUM_BRIGHTNESS_STATES total),
// then 5.. = self-running animations. Pressing the encoder cycles through all of them.
const byte NUM_BRIGHTNESS_STATES = NUM_LIGHTS + 1;
byte gSelected = 0;
bool gButtonLast = false;
unsigned long gButtonLastTime = 0;

// MIDI+MANUAL self-running animations (selected after the brightness states).
enum ManualAnim {
  MANIM_COMET,     // bright spot sweeps with a fading tail
  MANIM_SINE,      // phase-offset sine wave rolls across the bulbs
  MANIM_BREATHE,   // all bulbs fade up/down together
  MANIM_LARSON,    // single bright bulb bounces back and forth
  MANIM_TWINKLE,   // random bulbs sparkle and fade
  NUM_MANUAL_ANIMS
};
int   gManualSpeed[NUM_MANUAL_ANIMS] = {128, 128, 128, 128, 128};  // per-anim, 0..255
unsigned long gManualFrameAt = 0;
int   gManualPos = 0;              // chase / larson position
int   gManualDir = 1;              // larson direction
float gManualPhase = 0;            // sine / breathe phase
bool  gManualAnimStripDirty = true;

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

// MIDI clock / beat tracking (MIDI+MANUAL mode).
const byte PPQN = 24;              // MIDI clock pulses per quarter note
byte gClockCount = 0;
bool gClockOn = false;             // clock pulse toggle state

// Audio animations.
enum AudioAnim {
  ANIM_AMP_BRIGHT,   // 1: amplitude -> brightness        (param = sensitivity)
  ANIM_AMP_LPF,      // 2: amplitude -> brightness w/ LPF  (param = cutoff)
  ANIM_SPECTRUM,     // 3: 4-band spectrum split           (param = sensitivity)
  ANIM_BEAT,         // 4: beat pulse                      (param = sensitivity)
  ANIM_BREATHING,    // 5: smoothed loudness "breathing"   (param = smoothing)
  ANIM_SIDECHAIN,    // 6: duck (dim on loud)              (param = depth)
  NUM_AUDIO_ANIMS
};
byte gAudioAnim = 0;
int  gAnimParam[NUM_AUDIO_ANIMS] = {128, 128, 128, 128, 128, 128};  // per-anim, 0..255

// Audio envelope follower (non-blocking: one sample per loop, summarised per window).
int gAudioMin = 1023;
int gAudioMax = 0;
unsigned long gAudioWindowStart = 0;
byte gAudioLevel = 0;             // single-bulb output level (anims 1, 2, 6)
int  gLpfState = 512;             // one-pole low-pass state (anim 2)

// Spectrum split (anim 3): DC blocker + 3 crossover low-passes + per-band peaks.
int  gDc = 512;
int  gLp1 = 0, gLp2 = 0, gLp3 = 0;
int  gBandPeak[4]  = {0, 0, 0, 0};
byte gBandLevel[4] = {0, 0, 0, 0};

// Beat pulse (anim 4).
int  gBassLp = 0;
int  gBeatPeak = 0;
long gBeatAvg = 0;
byte gBeatLevel = 0;

// Breathing (anim 5).
int  gBreathLevel = 0;

// Audio strip display (shows the animation number, briefly the parameter on a turn).
bool gAudioStripDirty = true;
bool gShowingParam = false;
unsigned long gAudioParamUntil = 0;

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

  gMode = readModeSwitch();
  onModeChange();
}

// ----------------------------------------------------------------------------------
// >x< LOOP >x<
// ----------------------------------------------------------------------------------
void loop() {
  TopMode mode = readModeSwitch();
  if (mode != gMode) {
    gMode = mode;
    onModeChange();
  }

  // The encoder is polled every loop in both modes (the decoder needs frequent reads).
  handleEncoder();
  handleButton();

  if (gMode == MODE_MIDI_MANUAL) {
    MIDI.read();                                  // notes/CC/clock drive the bulbs
    if (gSelected < NUM_BRIGHTNESS_STATES) {      // brightness control (ALL / 1-4)
      if (gStripDirty) {
        drawManualStrip();
        gStripDirty = false;
      }
    } else {                                      // a self-running animation owns the bulbs
      updateManualAnim();
      if (gManualAnimStripDirty) {
        drawManualAnimStrip();
        gManualAnimStripDirty = false;
      }
    }
  } else {                                        // MODE_AUDIO (MIDI ignored)
    updateAudio();                                // current animation drives the bulbs
    updateAudioStrip();                           // animation number / parameter
  }
}

TopMode readModeSwitch() {
  return (digitalRead(SWITCH_PIN) == AUDIO_MODE_LEVEL) ? MODE_AUDIO : MODE_MIDI_MANUAL;
}

// Called once whenever the switch flips between modes.
void onModeChange() {
  gEncoderState = R_START;            // start the decoder fresh
  if (gMode == MODE_MIDI_MANUAL) {
    gSelected = 0;                    // default to ALL lights
    gStripDirty = true;              // redraw selection + brightness bar
  } else {
    resetAudioEnvelope();
    gShowingParam = false;
    gAudioStripDirty = true;         // draw the animation number
  }
}

// ----------------------------------------------------------------------------------
// DIMMER HELPERS
// ----------------------------------------------------------------------------------
void setLight(byte index, int value) {
  value = constrain(value, 0, 255);
  gBrightness[index] = (byte)value;
  gLights[index]->setBrightness((byte)value);
  gStripDirty = true;               // keep the MIDI+MANUAL bar in sync with any source
}

void setAllLights(int value) {
  for (byte i = 0; i < NUM_LIGHTS; i++) {
    setLight(i, value);
  }
}

// ----------------------------------------------------------------------------------
// ROTARY ENCODER (both modes)
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
  // DIR_CW maps to a left turn on this wiring, so right (DIR_CCW) increases.
  bool right = (dir == DIR_CCW);

  if (gMode == MODE_MIDI_MANUAL) {
    int delta = right ? ENCODER_STEP : -ENCODER_STEP;
    if (gSelected >= NUM_BRIGHTNESS_STATES) {             // animation: adjust speed
      byte a = gSelected - NUM_BRIGHTNESS_STATES;
      gManualSpeed[a] = constrain(gManualSpeed[a] + delta, 0, 255);  // right = faster
    } else if (gSelected == 0) {                          // ALL selected: brighten
      for (byte i = 0; i < NUM_LIGHTS; i++) setLight(i, gBrightness[i] + delta);
    } else {                                              // single bulb: brighten
      setLight(gSelected - 1, gBrightness[gSelected - 1] + delta);
    }
  } else {                                                // AUDIO: adjust anim parameter
    int delta = right ? AUDIO_PARAM_STEP : -AUDIO_PARAM_STEP;   // right = more
    gAnimParam[gAudioAnim] = constrain(gAnimParam[gAudioAnim] + delta, 0, 255);
    showAudioParam();
  }
}

void handleButton() {
  bool pressed = (digitalRead(RE_BUTTON_PIN) == LOW);
  if (pressed && !gButtonLast && (millis() - gButtonLastTime > BUTTON_DEBOUNCE_MS)) {
    gButtonLastTime = millis();
    if (gMode == MODE_MIDI_MANUAL) {
      // ALL, 1, 2, 3, 4, then the self-running animations.
      gSelected = (gSelected + 1) % (NUM_BRIGHTNESS_STATES + NUM_MANUAL_ANIMS);
      if (gSelected >= NUM_BRIGHTNESS_STATES) {
        resetManualAnim();
        gManualAnimStripDirty = true;
      } else {
        gStripDirty = true;
      }
    } else {
      gAudioAnim = (gAudioAnim + 1) % NUM_AUDIO_ANIMS;    // next animation
      resetAudioEnvelope();
      gShowingParam = false;
      gAudioStripDirty = true;
    }
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
  // Fill from the high-index end so the bar grows from the opposite physical side.
  for (byte i = 0; i < NUM_PIXELS; i++) {
    ledStrip.setPixelColor(i, i >= (NUM_PIXELS - litCount) ? color : 0);
  }
  ledStrip.show();
}

// Color used to indicate which light is selected in MIDI+MANUAL mode.
// Selection index: 0 = ALL, 1..4 = light 1..4.
uint32_t selectionColor(byte sel) {
  switch (sel) {
    case 1:  return ledStrip.Color(255,   0,   0);  // light 1 - red
    case 2:  return ledStrip.Color(  0, 255,   0);  // light 2 - green
    case 3:  return ledStrip.Color(  0,   0, 255);  // light 3 - blue
    case 4:  return ledStrip.Color(255, 150,   0);  // light 4 - amber
    default: return ledStrip.Color(255, 255, 255);  // ALL      - white
  }
}

// MIDI+MANUAL: color = selected light, lit pixels = its brightness.
void drawManualStrip() {
  byte b;
  if (gSelected == 0) {                   // ALL selected
    int sum = 0;
    for (byte i = 0; i < NUM_LIGHTS; i++) sum += gBrightness[i];
    b = sum / NUM_LIGHTS;
  } else {
    b = gBrightness[gSelected - 1];
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
// MIDI+MANUAL SELF-RUNNING ANIMATIONS (no audio / MIDI input needed)
// ----------------------------------------------------------------------------------
void resetManualAnim() {
  gManualPos = 0;
  gManualDir = 1;
  gManualPhase = 0;
  gManualFrameAt = millis();
  setAllLights(0);              // start from black
}

// Fade every bulb toward 0 (the tail for comet / larson / twinkle).
void manualDecayAll() {
  for (byte i = 0; i < NUM_LIGHTS; i++) {
    setLight(i, gBrightness[i] - (gBrightness[i] >> MANUAL_TAIL_SHIFT));
  }
}

// Advance and render the selected animation on a per-frame timer.
void updateManualAnim() {
  byte a = gSelected - NUM_BRIGHTNESS_STATES;
  int interval = map(gManualSpeed[a], 0, 255, MANUAL_FRAME_MAX_MS, MANUAL_FRAME_MIN_MS);
  if (millis() - gManualFrameAt < (unsigned long)interval) {
    return;
  }
  gManualFrameAt = millis();

  switch (a) {
    case MANIM_COMET: {
      manualDecayAll();
      gManualPos = (gManualPos + 1) % NUM_LIGHTS;
      setLight(gManualPos, 255);
      break;
    }
    case MANIM_SINE: {
      gManualPhase += MANUAL_PHASE_STEP;
      if (gManualPhase > TWO_PI) gManualPhase -= TWO_PI;
      for (byte i = 0; i < NUM_LIGHTS; i++) {
        float s = sin(gManualPhase + i * (TWO_PI / NUM_LIGHTS));
        setLight(i, (int)((s * 0.5f + 0.5f) * 255));
      }
      break;
    }
    case MANIM_BREATHE: {
      gManualPhase += MANUAL_PHASE_STEP;
      if (gManualPhase > TWO_PI) gManualPhase -= TWO_PI;
      setAllLights((int)((sin(gManualPhase) * 0.5f + 0.5f) * 255));
      break;
    }
    case MANIM_LARSON: {
      manualDecayAll();
      gManualPos += gManualDir;
      if (gManualPos >= NUM_LIGHTS - 1) { gManualPos = NUM_LIGHTS - 1; gManualDir = -1; }
      else if (gManualPos <= 0)        { gManualPos = 0;              gManualDir =  1; }
      setLight(gManualPos, 255);
      break;
    }
    case MANIM_TWINKLE: {
      manualDecayAll();
      setLight(random(NUM_LIGHTS), 255);
      break;
    }
  }
}

// Strip shows the animation number (anim+1 pixels) in a distinct hue.
void drawManualAnimStrip() {
  byte a = gSelected - NUM_BRIGHTNESS_STATES;
  fillStrip(colorWheel((byte)(a * 40 + 160)), a + 1);
}

// ----------------------------------------------------------------------------------
// MIDI NOTE / CC HANDLERS (MIDI+MANUAL mode)
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
// MIDI CLOCK HANDLERS (MIDI+MANUAL mode)
// 24 clock pulses per quarter note. We act once per beat.
// ----------------------------------------------------------------------------------
void handleClock() {
  if (gMode != MODE_MIDI_MANUAL) return;
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
}

void onBeat() {
  // Optional: pulse the bulbs on the beat (simple on/off). Expand with more patterns.
  if (gClockAnim) {
    gClockOn = !gClockOn;
    setAllLights(gClockOn ? 255 : 0);
  }
}

// ----------------------------------------------------------------------------------
// AUDIO MODE
// ----------------------------------------------------------------------------------
void resetAudioEnvelope() {
  gAudioMin = 1023;
  gAudioMax = 0;
  gAudioLevel = 0;
  gAudioWindowStart = millis();
  gLpfState = 512;
  gDc = 512;
  gLp1 = gLp2 = gLp3 = 0;
  for (byte i = 0; i < 4; i++) { gBandPeak[i] = 0; gBandLevel[i] = 0; }
  gBassLp = 0;
  gBeatPeak = 0;
  gBeatAvg = 0;
  gBeatLevel = 0;
  gBreathLevel = 0;
}

// One-pole IIR: state moves toward input by alpha/256 each step.
int onePole(int state, int input, int alpha) {
  return state + (int)(((long)(input - state) * alpha) >> 8);
}

void resetMinMax() {
  gAudioMin = 1023;
  gAudioMax = 0;
}

// Map this window's peak-to-peak loudness to a level with fast attack / slow release,
// updating the shared single-bulb level. Used by the amplitude-style animations.
byte envFollow(int pp, int ppFull) {
  if (ppFull <= AUDIO_PP_MIN) ppFull = AUDIO_PP_MIN + 1;
  byte target = (pp > AUDIO_PP_MIN)
                  ? (byte)constrain(map(pp, AUDIO_PP_MIN, ppFull, 0, 255), 0, 255)
                  : 0;
  if (target >= gAudioLevel) {
    gAudioLevel = target;                                                   // fast attack
  } else {
    gAudioLevel = (gAudioLevel > AUDIO_RELEASE) ? gAudioLevel - AUDIO_RELEASE : 0;  // release
  }
  return gAudioLevel;
}

// Non-blocking: sample once per loop (per-animation conditioning), then once per window
// turn the gathered features into bulb brightness.
void updateAudio() {
  int sample = analogRead(AUDIO_PIN);

  // ---- per-sample conditioning ----
  switch (gAudioAnim) {
    case ANIM_AMP_LPF: {
      int alpha = map(gAnimParam[ANIM_AMP_LPF], 0, 255, 4, 256);  // right = higher cutoff
      gLpfState = onePole(gLpfState, sample, alpha);
      if (gLpfState > gAudioMax) gAudioMax = gLpfState;
      if (gLpfState < gAudioMin) gAudioMin = gLpfState;
      break;
    }
    case ANIM_SPECTRUM: {
      gDc = onePole(gDc, sample, DC_ALPHA);
      int ac = sample - gDc;                       // bias removed
      gLp1 = onePole(gLp1, ac, SPLIT_ALPHA1);
      gLp2 = onePole(gLp2, ac, SPLIT_ALPHA2);
      gLp3 = onePole(gLp3, ac, SPLIT_ALPHA3);
      int e;
      e = abs(gLp1);          if (e > gBandPeak[0]) gBandPeak[0] = e;  // bass
      e = abs(gLp2 - gLp1);   if (e > gBandPeak[1]) gBandPeak[1] = e;  // low-mid
      e = abs(gLp3 - gLp2);   if (e > gBandPeak[2]) gBandPeak[2] = e;  // high-mid
      e = abs(ac - gLp3);     if (e > gBandPeak[3]) gBandPeak[3] = e;  // treble
      break;
    }
    case ANIM_BEAT: {
      gDc = onePole(gDc, sample, DC_ALPHA);
      gBassLp = onePole(gBassLp, sample - gDc, BEAT_BASS_ALPHA);
      int e = abs(gBassLp);
      if (e > gBeatPeak) gBeatPeak = e;
      break;
    }
    default: {                                     // AMP_BRIGHT, BREATHING, SIDECHAIN
      if (sample > gAudioMax) gAudioMax = sample;
      if (sample < gAudioMin) gAudioMin = sample;
      break;
    }
  }

  if (millis() - gAudioWindowStart < AUDIO_WINDOW_MS) {
    return;
  }
  gAudioWindowStart = millis();

  // ---- per-window output ----
  switch (gAudioAnim) {
    case ANIM_AMP_BRIGHT: {
      int pp = gAudioMax - gAudioMin; resetMinMax();
      int ppFull = map(gAnimParam[ANIM_AMP_BRIGHT], 0, 255, AUDIO_PP_FULL_MAX, AUDIO_PP_FULL_MIN);
      setAllLights(envFollow(pp, ppFull));
      break;
    }
    case ANIM_AMP_LPF: {
      int pp = gAudioMax - gAudioMin; resetMinMax();
      setAllLights(envFollow(pp, AUDIO_PP_FULL_LPF));
      break;
    }
    case ANIM_SPECTRUM: {
      int ppFull = map(gAnimParam[ANIM_SPECTRUM], 0, 255,
                       AUDIO_PP_FULL_SPLIT_MAX, AUDIO_PP_FULL_SPLIT_MIN);
      if (ppFull <= AUDIO_PP_MIN) ppFull = AUDIO_PP_MIN + 1;
      for (byte i = 0; i < NUM_LIGHTS; i++) {
        byte t = (gBandPeak[i] > AUDIO_PP_MIN)
                   ? (byte)constrain(map(gBandPeak[i], AUDIO_PP_MIN, ppFull, 0, 255), 0, 255)
                   : 0;
        gBandLevel[i] = (t >= gBandLevel[i]) ? t
                          : (gBandLevel[i] > AUDIO_RELEASE ? gBandLevel[i] - AUDIO_RELEASE : 0);
        setLight(i, gBandLevel[i]);
        gBandPeak[i] = 0;
      }
      break;
    }
    case ANIM_BEAT: {
      int energy = gBeatPeak; gBeatPeak = 0;
      long prevAvg = gBeatAvg;
      gBeatAvg = onePole((int)gBeatAvg, energy, BEAT_AVG_ALPHA);   // adapt slowly
      int factor16 = map(gAnimParam[ANIM_BEAT], 0, 255, 28, 17);   // right = more sensitive
      if (energy > BEAT_PP_MIN && (long)energy * 16 > prevAvg * factor16) {
        gBeatLevel = 255;                                          // beat -> snap bright
      } else {
        gBeatLevel = (gBeatLevel > BEAT_DECAY) ? gBeatLevel - BEAT_DECAY : 0;  // fade
      }
      setAllLights(gBeatLevel);
      break;
    }
    case ANIM_BREATHING: {
      int pp = gAudioMax - gAudioMin; resetMinMax();
      int target = (pp > AUDIO_PP_MIN)
                     ? constrain(map(pp, AUDIO_PP_MIN, AUDIO_PP_FULL_BREATH, 0, 255), 0, 255) : 0;
      int alpha = map(gAnimParam[ANIM_BREATHING], 0, 255, 64, 4);  // right = smoother/slower
      gBreathLevel = onePole(gBreathLevel, target, alpha);
      setAllLights(gBreathLevel);
      break;
    }
    case ANIM_SIDECHAIN: {
      int pp = gAudioMax - gAudioMin; resetMinMax();
      int loud = (pp > AUDIO_PP_MIN)
                   ? constrain(map(pp, AUDIO_PP_MIN, AUDIO_PP_FULL_DUCK, 0, 255), 0, 255) : 0;
      int duck = (int)((long)loud * gAnimParam[ANIM_SIDECHAIN] / 255);   // right = deeper duck
      int target = 255 - duck;
      if (target < gAudioLevel) {
        gAudioLevel = target;                                     // dip fast on loud
      } else {
        gAudioLevel = min(255, gAudioLevel + SIDE_RELEASE);       // recover slowly
      }
      setAllLights(gAudioLevel);
      break;
    }
  }
}

// A distinct hue per animation, so the number is also color-coded.
uint32_t audioAnimColor(byte anim) {
  return colorWheel((byte)(anim * 60 + 10));
}

void showAudioParam() {
  gShowingParam = true;
  gAudioParamUntil = millis() + AUDIO_PARAM_SHOW_MS;
  gAudioStripDirty = true;
}

// Strip shows the animation number (anim+1 pixels). On an encoder turn it shows the
// parameter level as a bar for AUDIO_PARAM_SHOW_MS, then reverts to the number.
void updateAudioStrip() {
  if (gShowingParam && millis() >= gAudioParamUntil) {
    gShowingParam = false;
    gAudioStripDirty = true;
  }
  if (!gAudioStripDirty) {
    return;
  }
  gAudioStripDirty = false;

  if (gShowingParam) {
    byte lit = map(gAnimParam[gAudioAnim], 0, 255, 0, NUM_PIXELS);
    fillStrip(ledStrip.Color(0, 80, 255), lit);            // parameter level = blue bar
  } else {
    fillStrip(audioAnimColor(gAudioAnim), gAudioAnim + 1); // animation number
  }
}
