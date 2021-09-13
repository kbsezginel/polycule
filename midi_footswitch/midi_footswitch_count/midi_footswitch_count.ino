#include <MIDI.h>
#include <Adafruit_NeoPixel.h>

// MIDI Setup
MIDI_CREATE_DEFAULT_INSTANCE();
// NEOPIXEL Setup
byte NUM_PIXELS = 8;
Adafruit_NeoPixel neoPixel(NUM_PIXELS, 2, NEO_GRB + NEO_KHZ800);
// Global Variables
int pressCount = 0;
int pressTol = 50;
int tolerance = 4;
int pressValue = 0;
int releaseValue = 0;
int colorIdx = 0;
int pitch = 60;
bool sendMidi = true;
bool noteOn = false;
int switch0 = 1014;
int switch1 = 905;
int switch2 = 813;
int switch3 = 724;
int switch4 = 626;
int switch5 = 500;
int switch6 = 321;


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
  if (sensorValue - tolerance < switch1 && sensorValue + tolerance > switch1) {
    // 1
    pressCount += 1;
    if (pressCount > pressTol) {
      lightNeoPixel(7, 1, colorIdx);
      // pitch = 36;
      if (!noteOn) {
        // MIDI.sendNoteOn(pitch, 127, 1);
        MIDI.sendStart();
        noteOn = true;
      }
      delay(10);
    }
  } else if (sensorValue - tolerance < switch2 && sensorValue + tolerance > switch2) {
    // 2
    pressCount += 1;
    if (pressCount > pressTol) {
      lightNeoPixel(6, 1, colorIdx);
      if (!noteOn) {
        // MIDI.sendNoteOn(pitch, 127, 2);
        MIDI.sendStop();
        noteOn = true;
      }
      delay(10);
    }
  } else if (sensorValue - tolerance < switch3 && sensorValue + tolerance > switch3) {
    // 3
    pressCount += 1;
    if (pressCount > pressTol) {
      lightNeoPixel(5, 1, colorIdx);
      if (!noteOn) {
        MIDI.sendNoteOn(pitch, 127, 3);
        noteOn = true;
        delay(10);
      }
    }
  } else if (sensorValue - tolerance < switch4 && sensorValue + tolerance > switch4) {
    // 4
    pressCount += 1;
    if (pressCount > pressTol) {
      lightNeoPixel(4, 1, colorIdx);
      if (!noteOn) {
        MIDI.sendNoteOn(pitch, 127, 4);
        noteOn = true;
        delay(10);
      }
    }
  } else if (sensorValue - tolerance < switch5 && sensorValue + tolerance > switch5) {
    // 5
    pressCount += 1;
    if (pressCount > pressTol) {
      lightNeoPixel(3, 1, colorIdx);
      if (!noteOn) {
        MIDI.sendNoteOn(pitch, 127, 5);
        noteOn = true;
        delay(10);
      }
    }
  } else if (sensorValue - tolerance < switch6 && sensorValue + tolerance > switch6) {
    // 6 - toggle
    pressCount += 1;
    if (pressCount > pressTol) {
      // MIDI.sendNoteOff(pitch, 0, 1);
      if (sendMidi) {
        lightNeoPixel(0, 8, 200);
      } else {
        lightNeoPixel(0, 8, 100);
      }
      sendMidi = !sendMidi;
      pressCount = 0;
      delay(500);
    }
  } else if (sensorValue - tolerance < switch0 && sensorValue + tolerance > switch0) {
    if (sendMidi) {
      clearNeoPixel();
      MIDI.sendNoteOff(pitch, 0, 1);
      noteOn = false;
    }
    pressCount = 0;
    // MIDI.sendNoteOff(pitch, 0, 1);
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
