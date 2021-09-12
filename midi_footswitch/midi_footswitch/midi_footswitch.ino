#include <MIDI.h>
#include <Adafruit_NeoPixel.h>

MIDI_CREATE_DEFAULT_INSTANCE();
byte NUM_PIXELS = 8;
Adafruit_NeoPixel neoPixel(NUM_PIXELS, 2, NEO_GRB + NEO_KHZ800);
int tolerance = 2;
int ledPin = 13;
unsigned long pressTime = 0.0;
unsigned long releaseTime = 0.0;
float timeDelay = 500;
int pressValue = 0;
int releaseValue = 0;
int rampTol = 5;
int colorIdx = 0;
int pitch = 42;

void setup() {
  MIDI.begin(1);   
  pinMode(ledPin, OUTPUT);
  neoPixel.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  neoPixel.show();            // Turn OFF all pixels ASAP
  neoPixel.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}

void loop() {
  int sensorValue = analogRead(A0);
  if (sensorValue - tolerance < 878 && sensorValue + tolerance > 878) {
    // 1
    if (abs(sensorValue - releaseValue) > rampTol) {
      clearNeoPixel();
      lightNeoPixel(7, 1, colorIdx);
      pitch = 42;
      MIDI.sendNoteOn(pitch, 127, 1);
    }
  } else if (sensorValue - tolerance < 731 && sensorValue + tolerance > 731) {
    // 2
    if (abs(sensorValue - releaseValue) > rampTol) {
      clearNeoPixel();
      lightNeoPixel(6, 1, colorIdx);
      pitch = 43;
      MIDI.sendNoteOn(pitch, 127, 2);
    }
  } else if (sensorValue - tolerance < 585 && sensorValue + tolerance > 585) {
    // 3
    if (abs(sensorValue - releaseValue) > rampTol) {
      clearNeoPixel();
      lightNeoPixel(5, 1, colorIdx);
      pitch = 44;
      MIDI.sendNoteOn(pitch, 127, 3);
    }
  } else if (sensorValue - tolerance < 438 && sensorValue + tolerance > 438) {
    // 4
    if (abs(sensorValue - releaseValue) > rampTol) {
      clearNeoPixel();
      lightNeoPixel(4, 1, colorIdx);
      pitch = 45;
      MIDI.sendNoteOn(pitch, 127, 4);
    }
  } else if (sensorValue - tolerance < 291 && sensorValue + tolerance > 291) {
    // 5
    if (abs(sensorValue - releaseValue) > rampTol) {
      clearNeoPixel();
      lightNeoPixel(3, 1, colorIdx);
      pitch = 46;
      MIDI.sendNoteOn(pitch, 127, 5);
    }
  } else if (sensorValue - tolerance < 145 && sensorValue + tolerance > 145) {
    // 6
    if (abs(sensorValue - releaseValue) > rampTol) {
      clearNeoPixel();
      MIDI.sendNoteOff(pitch, 0, 1);
    }
  } else {
    // releaseTime = millis();
    releaseValue = analogRead(A0);
    // MIDI.sendNoteOff(pitch, 0, 1);
  }
  // print out the value you read:
  // Serial.println(sensorValue);
  // delay(1);        // delay in between reads for stability
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
