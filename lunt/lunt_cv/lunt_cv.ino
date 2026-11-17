// ----------------------------------------------------------------------------------
// POLYCULE | LUNT CV | CV-reactive light controller
// ----------------------------------------------------------------------------------
// Arduino Nano + RobotDyn 4-channel AC dimmer + rotary encoder + 8px NeoPixel strip
// + 1/4" TRS control-voltage input (ring -> A2, sleeve -> GND, tip unused).
//
// Lunt with the ANIMATION mode replaced by a CV mode: an envelope follower (e.g. an
// Empress ZOIA sending CV out of its control port, fed by a drum mic) drives the bulbs,
// so they flash with the hits.
//
// The SWITCH chooses one of two modes:
//
//   * MANUAL (strip = warm yellow) - same as lunt.ino, minus the clock-indicator toggles
//       - Encoder press : cycle ALL -> 1 -> 2 -> 3 -> 4 -> STRIP.
//       - Encoder turn  : set the target's brightness (MIDI notes/CC also set bulbs).
//         STRIP = NeoPixel indicator brightness (applies to both modes).
//       - Strip: ALL = all pixels, Light N = the Nth pixel. Turning briefly shows a
//         brightness bar, then reverts to the pixel-number display.
//
//   * CV (strip = red level meter)
//       - All four bulbs follow the CV on A2: silence = FLOOR, full signal = full on.
//         The floor keeps the filaments warm so the bulbs respond faster to hits.
//       - Encoder turn  : adjust the selected setting.
//         FLOOR = bulb brightness with no signal (right = brighter).
//         SENS  = sensitivity: how little CV it takes to reach full (right = more).
//       - Encoder press : toggle the setting FLOOR (orange bar) <-> SENS (blue bar).
//       - Strip: a live red meter of the CV level (leftmost pixel alone = silence). While
//         you turn or press, it briefly shows the selected setting's bar instead.
//       - MIDI notes/CC are ignored.
// ----------------------------------------------------------------------------------
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>
#include "dimmable_light.h"

// ================================ CONFIG / KNOBS ==================================
// Switch level that selects CV mode. The switch is INPUT_PULLUP, so it idles
// HIGH (= MANUAL) and reads LOW when closed to GND (= CV). Flip if reversed.
#define CV_MODE_LEVEL      LOW

const int  ENCODER_STEP   = 8;     // MANUAL brightness change per encoder detent
const unsigned long BUTTON_DEBOUNCE_MS = 200;
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

// Strip feedback timing.
const unsigned long MANUAL_BRIGHT_SHOW_MS = 1200;  // brightness bar shown after a turn

// CV input. The ZOIA control port (set to CV in its Config Menu) sends 0-5 V on the RING
// of a TRS cable; the Nano reads 0-5 V as 0-1023 (~205 per volt). Bulb brightness:
//   reading <= CV_NOISE_GATE        -> FLOOR (silence)
//   reading >= full scale (SENS)    -> CV_BRIGHT_MAX
//   linear in between.
const int  CV_ADC_MAX       = 1023;  // 5 V
const int  CV_NOISE_GATE    = 20;    // readings at/below this (~0.1 V) count as silence
const int  CV_FULL_DEFAULT  = 1023;  // SENS at boot: the reading that gives full brightness
const int  CV_FULL_MIN      = 102;   // most sensitive SENS (~0.5 V = full brightness)
const int  CV_FULL_STEP     = 32;    // SENS change per encoder detent (~0.16 V)
const byte CV_FLOOR_DEFAULT = 40;    // FLOOR at boot: bulb brightness with no signal
const byte CV_FLOOR_MAX     = 160;   // the encoder can't raise the floor above this
const byte CV_FLOOR_STEP    = 4;     // FLOOR change per encoder detent
const byte CV_BRIGHT_MAX    = 255;   // bulb brightness at full CV
// The dimmer only acts once per mains half-cycle (8-10 ms), so a 2 ms update is plenty.
const unsigned long CV_UPDATE_MS = 2;           // read the CV / update the bulbs this often
// Each strip refresh briefly blocks interrupts (and so the dimmer timing): cap its rate.
const unsigned long CV_METER_REFRESH_MS = 40;   // meter redraw limit (~25 fps)
const unsigned long CV_SETTING_SHOW_MS = 1200;  // setting bar shown after a turn / press

static_assert(CV_FULL_MIN > CV_NOISE_GATE, "SENS full scale must stay above the noise gate");

// -------------------------------- CONTROLLER IO -----------------------------------
const byte SWITCH_PIN     = A0;   // mode switch  (D14)
const byte LED_PIN        = A1;   // NeoPixel data (D15)
const byte CV_PIN         = A2;   // CV in: TRS ring (sleeve -> GND)
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
uint32_t COLOR_RED;             // CV mode level meter
uint32_t COLOR_FLOOR_ORANGE;    // CV mode FLOOR setting bar
uint32_t COLOR_SENS_BLUE;       // CV mode SENS setting bar

// -------------------------------- STATE / RUNTIME ---------------------------------
enum Mode { MODE_MANUAL, MODE_CV };
Mode gMode = MODE_MANUAL;

bool gStripDirty = true;

// MANUAL targets, cycled by the encoder button:
//   0 = ALL, 1..4 = single light, STRIP = NeoPixel brightness.
const byte STRIP_TARGET       = NUM_LIGHTS + 1;   // = 5
const byte NUM_MANUAL_TARGETS = NUM_LIGHTS + 2;   // = 6
byte gSelected = 0;
bool gManualBrightActive = false;
unsigned long gManualBrightUntil = 0;
byte gStripBrightness = STRIP_BRIGHTNESS_DEFAULT;   // NeoPixel brightness, applies to all modes

// CV mode. Settings live in RAM and are kept across mode switches (reset on power-up).
enum CvTarget { CV_FLOOR, CV_SENS };
CvTarget gCvTarget = CV_FLOOR;     // setting the encoder adjusts
byte gCvFloor = CV_FLOOR_DEFAULT;  // bulb brightness with no signal
int  gCvFull = CV_FULL_DEFAULT;    // reading that gives full brightness (lower = more sensitive)
int  gCvOut = -1;                  // brightness last written to the bulbs (-1 = force a write)
unsigned long gCvReadAt = 0;       // last CV read
byte gCvMeter = 0;                 // meter level, 0..NUM_PIXELS-1 pixels after the leftmost
byte gCvMeterShown = 0xFF;         // meter level on the strip (0xFF = force a redraw)
unsigned long gCvMeterDrawnAt = 0;
bool gCvSettingActive = false;     // showing the FLOOR / SENS bar instead of the meter
unsigned long gCvSettingAt = 0;

// Encoder button.
bool gBtnDown = false;
unsigned long gBtnUpAt = 0;

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

// ----------------------------------------------------------------------------------
// >x< SETUP >x<
// ----------------------------------------------------------------------------------
void setup() {
  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);

  pinMode(RE_CLK_PIN, INPUT_PULLUP);
  pinMode(RE_DT_PIN, INPUT_PULLUP);
  pinMode(RE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(CV_PIN, INPUT);            // no pull-up: the CV source drives it

  ledStrip.begin();
  ledStrip.setBrightness(gStripBrightness);
  COLOR_WARM = ledStrip.Color(255, 130, 15);
  COLOR_RED  = ledStrip.Color(255, 0, 0);
  COLOR_FLOOR_ORANGE = ledStrip.Color(255, 90, 0);
  COLOR_SENS_BLUE    = ledStrip.Color(0, 0, 255);
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

  MIDI.read();           // notes/CC act only in MANUAL (gated in the handlers)
  handleEncoder();
  handleButton();

  if (gMode == MODE_MANUAL) {
    // Revert the brightness bar to the pixel-number display after a timeout.
    if (gManualBrightActive && millis() >= gManualBrightUntil) {
      gManualBrightActive = false;
      gStripDirty = true;
    }
  } else {
    updateCv();
    // Revert the setting bar to the meter after a timeout.
    if (gCvSettingActive && millis() - gCvSettingAt >= CV_SETTING_SHOW_MS) {
      gCvSettingActive = false;
      gStripDirty = true;
    }
    // Redraw the meter when its level changed, at most every CV_METER_REFRESH_MS.
    if (!gCvSettingActive && gCvMeter != gCvMeterShown &&
        millis() - gCvMeterDrawnAt >= CV_METER_REFRESH_MS) {
      gStripDirty = true;
    }
  }

  if (gStripDirty) {
    drawStrip();
    gStripDirty = false;
  }
}

Mode readModeSwitch() {
  return (digitalRead(SWITCH_PIN) == CV_MODE_LEVEL) ? MODE_CV : MODE_MANUAL;
}

void onModeChange() {
  gEncoderState = R_START;
  if (gMode == MODE_MANUAL) {
    gSelected = 0;
    gManualBrightActive = false;
    setAllLights(MANUAL_RESET_BRIGHTNESS);   // clean slate (no bulbs stuck from CV mode)
  } else {
    gCvSettingActive = false;
    gCvOut = -1;                    // write the bulbs on the first CV read
    gCvMeterShown = 0xFF;           // draw the meter right away
    gCvReadAt = millis() - CV_UPDATE_MS;
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
  } else if (gCvTarget == CV_FLOOR) {           // CV: right = brighter floor
    int step = right ? CV_FLOOR_STEP : -CV_FLOOR_STEP;
    gCvFloor = constrain(gCvFloor + step, 0, CV_FLOOR_MAX);
    showCvSetting();
  } else {                                      // CV: right = more sensitive (lower full scale)
    int step = right ? -CV_FULL_STEP : CV_FULL_STEP;
    gCvFull = constrain(gCvFull + step, CV_FULL_MIN, CV_ADC_MAX);
    showCvSetting();
  }
}

void handleButton() {
  bool pressed = (digitalRead(RE_BUTTON_PIN) == LOW);
  unsigned long now = millis();

  if (pressed && !gBtnDown && (now - gBtnUpAt > BUTTON_DEBOUNCE_MS)) {
    // --- press-down edge ---
    gBtnDown = true;
    if (gMode == MODE_MANUAL) {
      gSelected = (gSelected + 1) % NUM_MANUAL_TARGETS;   // ALL, 1-4, STRIP
      gManualBrightActive = false;
      gStripDirty = true;
    } else {
      gCvTarget = (gCvTarget == CV_FLOOR) ? CV_SENS : CV_FLOOR;
      showCvSetting();
    }
  } else if (!pressed && gBtnDown) {
    // --- release ---
    gBtnDown = false;
    gBtnUpAt = now;
  }
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
// CV INPUT
// ----------------------------------------------------------------------------------
// Read the CV and map it onto the bulbs: gate -> FLOOR, full scale -> CV_BRIGHT_MAX.
void updateCv() {
  unsigned long now = millis();
  if (now - gCvReadAt < CV_UPDATE_MS) return;
  gCvReadAt = now;

  int raw  = analogRead(CV_PIN);
  long span = gCvFull - CV_NOISE_GATE;                  // > 0 (see static_assert)
  long x    = constrain((long)raw - CV_NOISE_GATE, 0L, span);   // 0..span above the gate

  int b = gCvFloor + (int)((CV_BRIGHT_MAX - gCvFloor) * x / span);
  if (b != gCvOut) {                                    // only touch the dimmer on a change
    gCvOut = b;
    setAllLights(b);
  }

  // Meter: pixels after the leftmost one, rounded up so any signal above the gate shows.
  gCvMeter = (byte)((x * (NUM_PIXELS - 1) + span - 1) / span);
}

// Show the selected setting's bar on the strip for CV_SETTING_SHOW_MS.
void showCvSetting() {
  gCvSettingActive = true;
  gCvSettingAt = millis();
  gStripDirty = true;
}

// ----------------------------------------------------------------------------------
// NEOPIXEL STRIP
// ----------------------------------------------------------------------------------
// Logical pixel 0 is the far-left LED. The strip's data-in end is physically on the
// right, so map logical -> physical in reverse. The buffer helpers don't show();
// drawStrip() composites the content and then calls show() once.
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

void drawCvContent() {
  if (gCvSettingActive && gCvTarget == CV_FLOOR) {   // FLOOR bar: position in 0..CV_FLOOR_MAX
    fillPixels(COLOR_FLOOR_ORANGE, map(gCvFloor, 0, CV_FLOOR_MAX, 1, NUM_PIXELS));
  } else if (gCvSettingActive) {                     // SENS bar: fuller = more sensitive
    fillPixels(COLOR_SENS_BLUE, map(gCvFull, CV_ADC_MAX, CV_FULL_MIN, 1, NUM_PIXELS));
  } else {                                           // live CV meter (leftmost always lit)
    fillPixels(COLOR_RED, 1 + gCvMeter);
    gCvMeterShown = gCvMeter;
    gCvMeterDrawnAt = millis();
  }
}

void drawStrip() {
  if (gMode == MODE_MANUAL) {
    drawManualContent();
  } else {
    drawCvContent();
  }
  ledStrip.show();
}
