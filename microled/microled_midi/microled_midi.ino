// ----------------------------------------------------------------------------------
// POLYCULE | MICROLED | LED Animation
// ----------------------------------------------------------------------------------
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

#define POT_PIN 3
#define NEO_PIN 13
#define LED_PIN 2
#define SWITCH_PIN 3

// ---------------------------------- MIDI SETUP ------------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 14

// ---------------------------- NEOPIXEL LED STRIP SETUP ----------------------------
const byte NUM_PIXELS = 50;
Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
byte gLedStep = 5;
byte gLedAnimIdx = 0;
byte gLedNum = 0;
byte gColorIdx = 0;
byte gColorRed = 50;
byte gColorGreen = 0;
byte gColorBlue = 0;
uint32_t gColor = neoPixel.Color(gColorGreen, gColorRed, gColorBlue);
int gPixStart = 0;
int gPixEnd = 50;

#define NUM_COLORS 8
int gColorScheme1[NUM_COLORS] = {75, 75, 50, 50, 60, 60, 50, 50};
int gColorScheme2[NUM_COLORS] = {90, 90, 110, 110, 130, 130, 150, 150};
int gColorScheme3[NUM_COLORS] = {170, 170, 210, 210, 230, 230, 250, 250};
int gColorScheme[NUM_COLORS];

// ---------------------------- 7 SEGMENT DISPLAY SETUP -----------------------------
// SCL (A4) | SDA (A5)
Adafruit_7segment ledDisplay = Adafruit_7segment();

// ---------------------------------- TEMPO SETUP -----------------------------------
int gBpm = 120;
float gTimeDelay = 60.0 / gBpm * 1000.0;
unsigned long gPreviousTime = 0.0;
unsigned long gCurrentTime = 0.0;

// ------------------------------ POTENTIOMETER SETUP -------------------------------
int gPotVal = 0;

// --------------------------------- LED SETUP -------------------------------
int gLedState = LOW;

// --------------------------------- LED TEMPO SETUP -------------------------------
#define LED_ARRAY_PIN 12
Adafruit_NeoPixel ledArray = Adafruit_NeoPixel(8, LED_ARRAY_PIN, NEO_GRB + NEO_KHZ800);
byte gLedArrayIdx = 0;
int gLedArrayColorIdx = 0;

// ----------------------------------------------------------------------------------
// >x< SETUP >x<
// ----------------------------------------------------------------------------------
void setup() {
  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleCCFunction);

  neoPixel.begin();
  neoPixel.setBrightness(50);  // btw 0 - 127
  lightneoPixel(0, NUM_PIXELS, 0);

  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);
  ledDisplay.begin(0x70);
  for(int i = 0; i < 8; i++){
    gColorScheme[i] = gColorScheme1[i];
  }
}

// ----------------------------------------------------------------------------------
// >x< LOOP >x<
// ----------------------------------------------------------------------------------

void loop() {
  // setBPM(POT_PIN);
  // gCurrentTime = millis();
  if (digitalRead(SWITCH_PIN) == HIGH) {
    MIDI.read();
  } else {
    gCurrentTime = millis();
    if (gCurrentTime - gPreviousTime > gTimeDelay){
      blinkLED();
      setBPM(POT_PIN);
      animateNeoPixel3();
      gPreviousTime = gCurrentTime;
    }
  }

  // if (gCurrentTime - gPreviousTime > gTimeDelay){
  //   blinkLED();
  //   if (digitalRead(SWITCH_PIN) == HIGH) {
  //     setBPM(POT_PIN);
  //     animateNeoPixel3();
  //   } else {
  //     setColorScheme(POT_PIN);
  //     animateNeoPixel2();
  //   }
  //   gPreviousTime = gCurrentTime;
  // }
}
// ----------------------------------------------------------------------------------
void setBPM(int potPin) {
  int potRead = analogRead(potPin);
  gBpm = map(potRead, 0, 1023, 50, 250);
  gTimeDelay = 60.0 / gBpm * 500.0;
  ledDisplay.print(gBpm, DEC);
  ledDisplay.writeDisplay();
  gPotVal = potRead;
}

void setColorScheme(int potPin) {
  int potRead = analogRead(potPin);
  // Change value only if current read is different than previous set value
  byte csIdx = map(potRead, 0, 1023, 1, 4);
  if (csIdx == 1) {
    for(int i = 0; i < 8; i++){
      gColorScheme[i] = gColorScheme1[i];
    }
  }
  if (csIdx == 2) {
    for(int i = 0; i < 8; i++){
      gColorScheme[i] = gColorScheme2[i];
    }
  }
  if (csIdx == 3) {
    for(int i = 0; i < 8; i++){
      gColorScheme[i] = gColorScheme3[i];
    }
  }
  ledDisplay.print(csIdx, DEC);
  ledDisplay.writeDisplay();
  gPotVal = potRead;
}


// LED Array -----------------------------------------------------------------------
void animateLedArray() {
  gLedArrayIdx += 1;
  if (gLedArrayIdx == 1){
    gLedArrayColorIdx = 100;
  } else {
    gLedArrayColorIdx = 0;
  }
  if (gLedArrayIdx > 8) {
    gLedArrayIdx = 0;
    clearLedArray();
  } else {
    lightLedArray(0, gLedArrayIdx, gLedArrayColorIdx);
  }
}

void clearLedArray() {
  for(int i = 0; i < 8; i++){
    ledArray.setPixelColor(i, 0x000000);
    ledArray.show();
  }
}

void lightLedArray(int startIndex, int nPixels, int colorIdx) {
  for(int i = startIndex; i < startIndex + nPixels; i++){
    ledArray.setPixelColor(i, colorWheel(colorIdx));
    ledArray.show();
  }
}
// ----------------------------------------------------------------------------------

// Blink LED
void blinkLED() {
  if (gLedState == HIGH) {
    gLedState = LOW;
  } else {
    gLedState = HIGH;
  }
  digitalWrite(LED_PIN, gLedState);
}

// NEOPIXEL -------------------------------------------------------------------------
// Animate NeoPixel Strip | Light colors step by step and change color according to color scheme
void animateNeoPixel() {
  // Set color
  gColorIdx += 1;
  if (gColorIdx >= NUM_COLORS) {
    gColorIdx = 0;
  }
  // Set anim
  gLedNum += gLedStep;
  if (gLedNum > NUM_PIXELS) {
    gLedNum = 1;
    clearneoPixel();
  }

  lightneoPixel(0, gLedNum, gColorScheme[gColorIdx]);
}

// NeoPixel Animation 2 | Blink all LEDs and change color
void animateNeoPixel2() {
  // Set color
  gColorIdx += 1;
  if (gColorIdx >= NUM_COLORS) {
    gColorIdx = 0;
  }
  // Set anim
  gLedAnimIdx += 1;
  if (gLedAnimIdx == 1) {
    lightneoPixel(0, NUM_PIXELS, gColorScheme[gColorIdx]);
  } else {
    clearneoPixel();
    gLedAnimIdx = 0;
  }
}

// NeoPixel Animation 3 | Light ever other LED and change color
void animateNeoPixel3() {
  clearneoPixel();
  // Set color
  gColorIdx += 1;
  if (gColorIdx >= NUM_COLORS) {
    gColorIdx = 0;
  }
  // Set anim
  byte ledStart = 0;
  gLedAnimIdx += 1;
  if (gLedAnimIdx == 1) {
    ledStart = 1;
  } else {
    gLedAnimIdx = 0;
  }
  for(int i = ledStart; i < NUM_PIXELS; i += 2){
    neoPixel.setPixelColor(i, colorWheel(gColorScheme[gColorIdx]));
    neoPixel.show();
  }
}

void clearneoPixel() {
  for(int i = 0; i < NUM_PIXELS; i++){
    neoPixel.setPixelColor(i, 0x000000);
    neoPixel.show();
  }
}

void lightneoPixel(int startIndex, int nPixels, int colorIdx) {
  for(int i = startIndex; i < startIndex + nPixels; i++){
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

void lightPixels(int startIndex, int nPixels, uint32_t color) {
  for(int i = startIndex; i < startIndex + nPixels; i++){
    neoPixel.setPixelColor(i, color);
    neoPixel.show();
  }
}


// ----------------------------------------------------------------------------------
// MIDI FUNCTIONS
// ----------------------------------------------------------------------------------
void handleNoteOn(byte channel, byte pitch, byte velocity) {
//  int bright = map(velocity, 0, 127, 0, 255);
//  neoPixel.setBrightness(bright);
    // byte ledNo = 60 - pitch;
    byte ledNo = map(pitch, 0, 127, 0, NUM_PIXELS);
    neoPixel.setPixelColor(ledNo, gColor);
    neoPixel.show();
  // lightPixels(0, NUM_PIXELS, gColor);
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
//  byte ledNo = 60 - pitch;
//  neoPixel.setPixelColor(ledNo, 0x000000);
//  neoPixel.show();
  clearneoPixel();
}

void handleCCFunction(byte channel, byte number, byte value) {
  switch (number) {
    case 22:
      gColorRed = map(value, 0, 127, 0, 255);
      gColor = neoPixel.Color(gColorGreen, gColorRed, gColorBlue);
      lightPixels(gPixStart, gPixEnd, gColor);
      break;
    case 23:
      gColorGreen = map(value, 0, 127, 0, 255);
      gColor = neoPixel.Color(gColorGreen, gColorRed, gColorBlue);
      break;
    case 24:
      gColorBlue = map(value, 0, 127, 0, 255);
      gColor = neoPixel.Color(gColorGreen, gColorRed, gColorBlue);
      break;
    case 25:
      byte bright = map(value, 0, 127, 0, 255);
      neoPixel.setBrightness(bright);
      break;
    case 20:
      gPixStart = map(value, 0, 127, 0, NUM_PIXELS);
      lightPixels(gPixStart, gPixEnd, gColor);
      break;
    case 21:
      gPixEnd = map(value, 0, 127, 0, NUM_PIXELS);
      lightPixels(gPixStart, gPixEnd, gColor);
      break;
  }
}
