#include <Adafruit_NeoPixel.h>

// NEOPIXEL Setup
byte NUM_PIXELS = 8;
Adafruit_NeoPixel neoPixel(NUM_PIXELS, 2, NEO_GRB + NEO_KHZ800);
// Global Variables
int pressCount = 0;
int pressTol = 10;
int tolerance = 2;
int pressValue = 0;
int releaseValue = 0;
int rampTol = 5;
int colorIdx = 0;
int pitch = 42;

// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT_PULLUP);
  // neoPixel.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  // neoPixel.show();            // Turn OFF all pixels ASAP
  // neoPixel.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}
// -----------------------------------------------------------------------------
void loop() {
  int sensorValue = analogRead(A0);
  // lightNeoPixel(0, n, 0);
  // print out the value you read:
  Serial.println(sensorValue);
  delay(20);        // delay in between reads for stability
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
