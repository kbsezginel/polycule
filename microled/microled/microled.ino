#include <Adafruit_NeoPixel.h>
#include <math.h>

#define MIC_PIN 5
#define LED_RING_PIN 13
int soundLevel = 0;

const byte NUM_PIXELS = 7;
Adafruit_NeoPixel ledRing = Adafruit_NeoPixel(NUM_PIXELS, LED_RING_PIN, NEO_GRB + NEO_KHZ800);
byte gColorIdx = 0;

const int baseLine = 530;
const int volMax = 300;
int ledLevel = 0;
int ledNum = 0;

void setup() {
  ledRing.begin();
  ledRing.setBrightness(50);  // btw 0 - 127
  lightLedRing(0, NUM_PIXELS, 0);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  soundLevel = analogRead(MIC_PIN);
  soundLevel = abs(soundLevel - baseLine);
  ledLevel = map(soundLevel, 0, volMax, 30, 126);
  ledNum = map(soundLevel, 0, volMax, 1, 7);
  ledRing.setBrightness(ledLevel);
  ledRing.show();
  lightLedRing(0, ledNum, 0);
  
  Serial.print("\nMIC val: ");
  Serial.print(ledLevel);
  delay(100);
  clearLedRing();
  delay(100);
}

void clearLedRing() {
  for(int i = 0; i<NUM_PIXELS; i++){
    ledRing.setPixelColor(i, 0x000000);
    ledRing.show();
  }
}

void lightLedRing(int startIndex, int nPixels, int colorIdx) {
  for(int i=startIndex;i<startIndex+nPixels;i++){
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

uint32_t dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}
