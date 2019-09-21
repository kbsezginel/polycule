// ----------------------------------------------------------------------------------
// POLYCULE | MICROLED | LED Animation with audio input
// ----------------------------------------------------------------------------------
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

#define POT_PIN 3
#define MIC_PIN 0
#define LED_RING_PIN 13
#define LED_PIN 12
int soundLevel = 0;

const byte NUM_PIXELS = 7;
Adafruit_NeoPixel ledRing = Adafruit_NeoPixel(NUM_PIXELS, LED_RING_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_7segment ledDisplay = Adafruit_7segment();

// The baseline depends on the voltage used for the mic amp (using 330 for 3.3 V)
const int baseLine = 330;
const int volMax = 400;
int gLedLevel = 0;
int gLedNum = 0;
byte gColorIdx = 0;
int gBpm = 120;
float gTimeDelay = 60.0 / gBpm * 1000.0;
int gPotVal = 0;
int gLedState = LOW;

unsigned long gPreviousTime = 0.0;
unsigned long gCurrentTime = 0.0;
// -------------------------------------------
void setup() {
  ledRing.begin();
  ledRing.setBrightness(50);  // btw 0 - 127
  lightLedRing(0, NUM_PIXELS, 0);
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  ledDisplay.begin(0x70);
}

void loop() {
  setBPM(POT_PIN);
  gCurrentTime = millis();

  //  soundLevel = analogRead(MIC_PIN);
  //  soundLevel -= baseLine;
  //  gLedLevel = map(soundLevel, 0, volMax, 30, 126);
  //  ledRing.setBrightness(gLedLevel);
  //  ledRing.show();

  //  Serial.print('\n');
  //  Serial.print(soundLevel);
  //  Serial.print(',');
  //  Serial.print(gColorIdx);

  if (gCurrentTime - gPreviousTime > gTimeDelay){
    if (gLedState == HIGH) {
      gLedState = LOW;
      gLedNum += 1;
      if (gLedNum > 8) {
        gLedNum = 1;
      }
      gColorIdx += 1;
      lightLedRing(0, gLedNum, gColorIdx);
    } else {
      gLedState = HIGH;
      clearLedRing();
    }
    digitalWrite(LED_PIN, gLedState);
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

void clearLedRing() {
  for(int i = 0; i < NUM_PIXELS; i++){
    ledRing.setPixelColor(i, 0x000000);
    ledRing.show();
  }
}

void lightLedRing(int startIndex, int nPixels, int colorIdx) {
  for(int i = startIndex; i < startIndex + nPixels; i++){
    ledRing.setPixelColor(i, colorWheel(colorIdx));
    ledRing.show();
  }
}

uint32_t colorWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return ledRing.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return ledRing.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return ledRing.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
