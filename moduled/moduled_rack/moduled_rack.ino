// ----------------------------------------------------------------------------------
// POLYCULE | MODULED Rack | Kutay B. Sezginel | June 2019
// ----------------------------------------------------------------------------------
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
// ----------------------------------- GLOBALS --------------------------------------
int gBpm = 120;
float gDelayTime = 60.0 / gBpm * 1000.0;
int gDelayTimeInt = gDelayTime;
// ---------------------------------- OLED SETUP ------------------------------------
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
unsigned long gDisplayTime = 0.0;
unsigned long gDisplayDelta = gDelayTime;
// ------------------------------ LARGE LED RING SETUP ------------------------------
const byte LARGE_LED_RING_PIN = 12;
const byte NUM_PIXELS_LARGE = 24;
Adafruit_NeoPixel ledRing1 = Adafruit_NeoPixel(NUM_PIXELS_LARGE, LARGE_LED_RING_PIN, NEO_GRB + NEO_KHZ800);
unsigned long gLargeRingDelta = gDelayTime;
// ------------------------------ SMALL LED RING SETUP ------------------------------
const byte SMALL_LED_RING_PIN = 11;
const byte NUM_PIXELS_SMALL = 12;
Adafruit_NeoPixel ledRing2 = Adafruit_NeoPixel(NUM_PIXELS_SMALL, SMALL_LED_RING_PIN, NEO_GRB + NEO_KHZ800);
unsigned long gSmallRingDelta = gDelayTime;

byte gColorIdx = 0;
byte gColorStep = 9;
byte gLedStep = 3;
byte gLedCounter = 0;
unsigned long gCurrentTime = 0.0;
// ----------------------------- POTENTIOMETER SETUP --------------------------------
const byte LEFT_POT_PIN = 0;
const byte RIGHT_POT_PIN = 1;
int gLeftPotVal = 500;
int gRightPotVal = 500;
// -------------------------------- SWITCH SETUP ------------------------------------
const byte SWITCH_PIN = 4;
// -------------------------------- BUTTON SETUP ------------------------------------
const byte BUTTON_PIN = 3;
// ----------------------------------------------------------------------------------
// >x< SETUP >x<
// ----------------------------------------------------------------------------------
void setup() {
  pinMode(SWITCH_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  ledRing1.begin();
  ledRing1.setBrightness(50);  // btw 0 - 127

  ledRing2.begin();
  ledRing2.setBrightness(50);  // btw 0 - 127

  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 20);
  display.println("MODULED!");
  display.display();
  delay(1000);
}

void loop() {
  setBPM();
  setColor();
  printDisplay();

  if (millis() - gCurrentTime > gSmallRingDelta){
    lightLedRing1(0, gLedCounter, gColorIdx);
  }
  if (millis() - gCurrentTime > gLargeRingDelta){
    lightLedRing2(0, gLedCounter, gColorIdx);
    gLedCounter += gLedStep;
    gColorIdx += gColorStep * gLedStep;
    gCurrentTime = millis();
  }

  if (gLedCounter > NUM_PIXELS_LARGE) {
    clearLedRing1();
    clearLedRing2();
    gLedCounter = gLedStep;
    gColorIdx = 0;
    gCurrentTime = millis();
  }
}

void setBPM() {
  int potRead = analogRead(LEFT_POT_PIN);
  // Change value only if current read is different than previous set value
  if (abs(gLeftPotVal - potRead) > 20) {
    gBpm = map(potRead, 0, 1023, 10, 300);
    gDelayTime = 60.0 / gBpm * 1000.0;
    gDelayTimeInt = gDelayTime;
  }
}

void setColor() {
  int potRead = analogRead(RIGHT_POT_PIN);
  // Change value only if current read is different than previous set value
  if (abs(gRightPotVal - potRead) > 20) {
    gColorIdx = map(potRead, 0, 1023, 0, 255);
  }
}

void printDisplay(){
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(" BPM | DLY");
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println(gBpm);
  display.setCursor(85, 20);
  display.println(gDelayTimeInt);
  display.setCursor(0, 40);
  display.setTextSize(1);
  int beatNum = ceil(millis() - gDisplayTime) / gDisplayDelta;
  if (beatNum == 1){
    display.println(">                   <");
  } else if (beatNum == 2) {
    display.println("> >               < <");
  } else if (beatNum == 3) {
    display.println("> > >           < < <");
  } else if (beatNum == 4) {
    display.println("> > > >   .   < < < <");
  } else if (beatNum > 4) {
    gDisplayTime = millis();
  }
  display.display();
}

// ----------------------------------------------------------------------------------
// LED RING FUNCTIONS
// ----------------------------------------------------------------------------------
void clearLedRing1() {
  for(int i = 0; i<NUM_PIXELS_LARGE; i++){
    ledRing1.setPixelColor(i, 0x000000);
    ledRing1.show();
  }
}

void lightLedRing1(int startIndex, int nPixels, int colorIdx) {
  for(int i=startIndex;i<startIndex+nPixels;i++){
    ledRing1.setPixelColor(i, colorWheel(colorIdx));
    ledRing1.show();
  }
}

uint32_t colorWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return ledRing1.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return ledRing1.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return ledRing1.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}

// LED Animation (KnightRider)
void ledAnim1(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color, bool clearAll) {
  uint32_t old_val[NUM_PIXELS_LARGE]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 0; count<NUM_PIXELS_LARGE + 1; count++) {
      ledRing1.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        ledRing1.setPixelColor(x-1, old_val[x-1]);
      }
      ledRing1.show();
      delay(speed);
    }
   for (int count = NUM_PIXELS_LARGE-1; count>=0; count--) {
     ledRing1.setPixelColor(count, color);
     old_val[count] = color;
     for(int x = count; x<=NUM_PIXELS_LARGE ;x++) {
       old_val[x-1] = dimColor(old_val[x-1], width);
       ledRing1.setPixelColor(x+1, old_val[x+1]);
     }
     ledRing1.show();
     delay(speed);
   }
  }
  if (clearAll){
    void clearLedRing1();
  }
}

// ----------------------------------------------------------------------------------
// LED RING FUNCTIONS
// ----------------------------------------------------------------------------------
void clearLedRing2() {
  for(int i = 0; i<NUM_PIXELS_SMALL; i++){
    ledRing2.setPixelColor(i, 0x000000);
    ledRing2.show();
  }
}

void lightLedRing2(int startIndex, int nPixels, int colorIdx) {
  for(int i=startIndex;i<startIndex+nPixels;i++){
    ledRing2.setPixelColor(i, colorWheel(colorIdx));
    ledRing2.show();
  }
}

// LED Animation (KnightRider)
void ledAnim2(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color, bool clearAll) {
  uint32_t old_val[NUM_PIXELS_SMALL]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 0; count<NUM_PIXELS_SMALL + 1; count++) {
      ledRing2.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        ledRing2.setPixelColor(x-1, old_val[x-1]);
      }
      ledRing2.show();
      delay(speed);
    }
   for (int count = NUM_PIXELS_SMALL-1; count>=0; count--) {
     ledRing2.setPixelColor(count, color);
     old_val[count] = color;
     for(int x = count; x<=NUM_PIXELS_SMALL ;x++) {
       old_val[x-1] = dimColor(old_val[x-1], width);
       ledRing2.setPixelColor(x+1, old_val[x+1]);
     }
     ledRing2.show();
     delay(speed);
   }
  }
  if (clearAll){
    void clearLedRing2();
  }
}
