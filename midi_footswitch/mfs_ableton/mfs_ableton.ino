// MIDI FOOTSWITCH ABLETON
// 3 sets of 5 midi notes to be mapped to anything in ableton
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>

// MIDI Setup
MIDI_CREATE_DEFAULT_INSTANCE();
// NEOPIXEL Setup
byte NUM_PIXELS = 8;
Adafruit_NeoPixel neoPixel(NUM_PIXELS, 2, NEO_GRB + NEO_KHZ800);
byte ledBrightness = 50;
// Mode Setup ------------------------------------------------------------------
byte NUM_MODES = 3;
byte NUM_SWITCHES = 5;
byte modeCount = 1;
int colors[] = {0, 85, 170};
byte colorIdx = 0;
// Note Setup
byte noteMidiChannel = 8;
int pitchStart = 30;
int pitch1 = 30;
int pitch2 = 35;
int pitch3 = 40;
int pitch4 = 45;
int pitch5 = 50;
bool noteOn = false;
bool note1On = false;
bool note2On = false;
bool note3On = false;
bool note4On = false;
bool note5On = false;
// Switch Setup
byte tolerance = 4;
byte pressTol = 50;
int pressCount = 0;
// Switch read values ----------------------------------------------------------
int switch0 = 1014;
int switch1 = 905;
int switch2 = 813;
int switch3 = 724;
int switch4 = 626;
int switch5 = 500;
int switch6 = 321;
int switches[] = {1014, 905, 813, 724, 626, 500, 321};
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void setup() {
  pinMode(A0, INPUT_PULLUP);
  MIDI.begin(1);
  neoPixel.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  neoPixel.show();            // Turn OFF all pixels ASAP
  neoPixel.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}
// -----------------------------------------------------------------------------
void loop() {
  int sensorValue = analogRead(A0);
  // int switchNumber = detectSwitch(sensorValue);

  if (sensorValue - tolerance < switch1 && sensorValue + tolerance > switch1) {
    // 1
    byte switchNum = 1;
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note1On) {
        for(int i = 1; i<NUM_MODES+1; i++){
          if (i == modeCount){
            lightNeoPixel(7, 1, colors[i - 1]);
            pitch1 = pitchStart + (modeCount - 1) * NUM_SWITCHES + switchNum;
            MIDI.sendNoteOn(pitch1, 127, noteMidiChannel);
            delay(50);
            break;
          }
        }
        note1On = true;

      }
    }
  } else if (sensorValue - tolerance < switch2 && sensorValue + tolerance > switch2) {
    // 2
    byte switchNum = 2;
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note2On) {
        for(int i = 1; i<NUM_MODES+1; i++){
          if (i == modeCount){
            lightNeoPixel(6, 1, colors[i - 1]);
            pitch2 = pitchStart + (modeCount - 1) * NUM_SWITCHES + switchNum;
            MIDI.sendNoteOn(pitch2, 127, noteMidiChannel);
            delay(50);
            break;
          }
        }
        note2On = true;

      }
    }
  } else if (sensorValue - tolerance < switch3 && sensorValue + tolerance > switch3) {
    // 3
    byte switchNum = 3;
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note3On) {

        for(int i = 1; i<NUM_MODES+1; i++){
          if (i == modeCount){
            lightNeoPixel(5, 1, colors[i - 1]);
            pitch3 = pitchStart + (modeCount - 1) * NUM_SWITCHES + switchNum;
            MIDI.sendNoteOn(pitch3, 127, noteMidiChannel);
            delay(50);
            break;
          }
        }
        note3On = true;
      }
    }
  } else if (sensorValue - tolerance < switch4 && sensorValue + tolerance > switch4) {
    // 4
    byte switchNum = 4;
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note4On) {

        for(int i = 1; i<NUM_MODES+1; i++){
          if (i == modeCount){
            lightNeoPixel(4, 1, colors[i - 1]);
            pitch4 = pitchStart + (modeCount - 1) * NUM_SWITCHES + switchNum;
            MIDI.sendNoteOn(pitch4, 127, noteMidiChannel);
            delay(50);
            break;
          }
        }
        note4On = true;
      }
    }
  } else if (sensorValue - tolerance < switch5 && sensorValue + tolerance > switch5) {
    // 5
    byte switchNum = 5;
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note5On) {

        for(int i = 1; i<NUM_MODES+1; i++){
          if (i == modeCount){
            lightNeoPixel(3, 1, colors[i - 1]);
            pitch5 = pitchStart + (modeCount - 1) * NUM_SWITCHES + switchNum;
            MIDI.sendNoteOn(pitch5, 127, noteMidiChannel);
            delay(50);
            break;
          }
        }
        note5On = true;
      }
    }
  } else if (sensorValue - tolerance < switch6 && sensorValue + tolerance > switch6) {
    // 6 - toggle
    pressCount += 1;
    if (pressCount > pressTol) {
      modeCount += 1;
      if (modeCount > NUM_MODES) {
        modeCount = 1;
      }

      for(int i = 1; i<NUM_MODES+1; i++){
        if (i == modeCount){
          lightNeoPixel(0, 8, colors[i - 1]);
        }
      }

      pressCount = 0;
      delay(500);
      clearNeoPixel();
    }
  } else if (sensorValue - tolerance < switch0 && sensorValue + tolerance > switch0) {
    // 0 - release
    if (noteOn) {
      MIDI.sendNoteOff(pitch1, 0, noteMidiChannel);
      MIDI.sendNoteOff(pitch2, 0, noteMidiChannel);
      MIDI.sendNoteOff(pitch3, 0, noteMidiChannel);
      MIDI.sendNoteOff(pitch4, 0, noteMidiChannel);
      MIDI.sendNoteOff(pitch5, 0, noteMidiChannel);
      noteOn = false;
      clearNeoPixel();
    }
    note1On = false;
    note2On = false;
    note3On = false;
    note4On = false;
    note5On = false;
    for(int i = 1; i<NUM_MODES+1; i++){
      if (i == modeCount){
        lightNeoPixel(0, 1, colors[i - 1]);
      }
    }
    pressCount = 0;
  }
  delay(1);        // delay in between reads for stability
}
// ----------------------------------------------------------------------------------
// LED FUNCTIONS
// ----------------------------------------------------------------------------------
void clearNeoPixel() {
  for(int i = 0; i<NUM_PIXELS; i++){
    neoPixel.setPixelColor(i, 0x000000);
    neoPixel.show();
  }
}

void clearPixel(byte i) {
  neoPixel.setPixelColor(i, 0x000000);
  neoPixel.show();
}

void lightNeoPixel(int startIndex, int nPixels, int colorIdx) {
  for(int i=startIndex;i<startIndex+nPixels;i++){
    neoPixel.setPixelColor(i, colorWheel(colorIdx));
    neoPixel.show();
  }
}

uint32_t colorWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return neoPixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return neoPixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return neoPixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}

byte detectSwitch(int sensorValue) {
  int switchNum = 10;
  for(int i=0; i < sizeof(switches) - 1; i++){
    if (sensorValue - tolerance < switches[i] && sensorValue + tolerance > switches[i]) {
      // Sensor value within tolerance of switch
      // Count number of presses to make sure it's legit
      pressCount += 1;
      if (pressCount > pressTol) {
        noteOn = true;
        switchNum = i;
      }
    }
  }
  return switchNum;
}
