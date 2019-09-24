// ----------------------------------------------------------------------------------
// POLYCULE | MICROLED | LED Animation
// ----------------------------------------------------------------------------------
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

#define POT_PIN 3
#define NEO_PIN 13
#define LED_PIN 2

// ---------------------------- NEOPIXEL LED STRIP SETUP ----------------------------
const byte NUM_PIXELS = 50;
Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
byte gLedStep = 5;
byte gColorIdx = 0;
int gLedLevel = 0;
int gLedNum = 0;
int gColorScheme[] = {0, 0, 50, 50, 200, 200, 50, 50};

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
  neoPixel.begin();
  neoPixel.setBrightness(50);  // btw 0 - 127
  lightneoPixel(0, NUM_PIXELS, 0);

  pinMode(LED_PIN, OUTPUT);
  ledDisplay.begin(0x70);
}

void loop() {
  setBPM(POT_PIN);
  gCurrentTime = millis();

  if (gCurrentTime - gPreviousTime > gTimeDelay){
    blinkLED();
    animateNeoPixel();
    animateLedArray();
    gPreviousTime = gCurrentTime;
  }
}
// -------------------------------------------------------
void setBPM(int potPin) {
  int potRead = analogRead(potPin);
  // Change value only if current read is different than previous set value
  if (abs(gPotVal - potRead) > 5) {
    gBpm = map(potRead, 0, 1023, 10, 300);
    gTimeDelay = 60.0 / gBpm * 500.0;
    ledDisplay.print(gBpm, DEC);
    ledDisplay.writeDisplay();
    gPotVal = potRead;
  }
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

// Animate NeoPixel Strip | Light colors step by step and change color according to color scheme
void animateNeoPixel() {
  gLedNum += gLedStep;
  gColorIdx += 1;
  if (gLedNum > NUM_PIXELS) {
    gLedNum = 1;
    clearneoPixel();
  }
  if (gColorIdx >= sizeof(gColorScheme)) {
    gColorIdx = 0;
  }
  lightneoPixel(0, gLedNum, gColorScheme[gColorIdx]);
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
