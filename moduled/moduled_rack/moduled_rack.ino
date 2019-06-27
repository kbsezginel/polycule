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
byte gDisplayMultiplier = 1;
// ------------------------------ LARGE LED RING SETUP ------------------------------
const byte LARGE_LED_RING_PIN = 12;
const byte NUM_PIXELS_LARGE = 24;
Adafruit_NeoPixel gLargeLedRing = Adafruit_NeoPixel(NUM_PIXELS_LARGE, LARGE_LED_RING_PIN, NEO_GRB + NEO_KHZ800);
unsigned long gLargeRingDelta = gDelayTime;
unsigned long gLargeRingTime = 0.0;
byte gLargeRingLedStep = 3;
byte gLargeRingCounter = 0;
byte gLargeRingColor = 0;
byte gLargeRingColorStep = 9;
byte gLargeRingMultiplier = 1;
// ------------------------------ SMALL LED RING SETUP ------------------------------
const byte SMALL_LED_RING_PIN = 11;
const byte NUM_PIXELS_SMALL = 12;
Adafruit_NeoPixel gSmallLedRing = Adafruit_NeoPixel(NUM_PIXELS_SMALL, SMALL_LED_RING_PIN, NEO_GRB + NEO_KHZ800);
unsigned long gSmallRingDelta = gDelayTime;
unsigned long gSmallRingTime = 0.0;
byte gSmallRingLedStep = 1;
byte gSmallRingCounter = 0;
byte gSmallRingColor = 0;
byte gSmallRingColorStep = 9;
byte gSmallRingMultiplier = 1;

unsigned long gCurrentTime = 0.0;
// ----------------------------- POTENTIOMETER SETUP --------------------------------
const byte LEFT_POT_PIN = 0;
const byte RIGHT_POT_PIN = 1;
int gLeftPotVal = 500;
int gRightPotVal = 500;
// -------------------------------- SWITCH SETUP ------------------------------------
const byte SWITCH_PIN = 4;
String gSwitchDisplay = " ";
// -------------------------------- BUTTON SETUP ------------------------------------
const byte BUTTON_PIN = 3;
String gButtonDisplay = " ";
// ----------------------------------------------------------------------------------
// >x< SETUP >x<
// ----------------------------------------------------------------------------------
void setup() {
  pinMode(SWITCH_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  gLargeLedRing.begin();
  gLargeLedRing.setBrightness(50);  // btw 0 - 127

  gSmallLedRing.begin();
  gSmallLedRing.setBrightness(50);  // btw 0 - 127

  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(" MODULED ");
  display.setCursor(0, 20);
  display.println(" RACK    ");
  display.setCursor(0, 40);
  display.println("==========");
  display.display();
  delay(3000);
}
// ----------------------------------------------------------------------------------
// >x< LOOP >x<
// ----------------------------------------------------------------------------------
void loop() {
  setBPM();
  setSwitch();
  setButton();
  setColor();
  printDisplay();

  gCurrentTime = millis();

  if (gCurrentTime - gSmallRingTime > gSmallRingDelta){
    lightSmallLedRing(0, gSmallRingCounter, gSmallRingColor);
    gSmallRingTime = millis();
    gSmallRingCounter += gSmallRingLedStep;
    gSmallRingColor += gSmallRingColorStep * gSmallRingLedStep;
  }

  if (gCurrentTime - gLargeRingTime > gLargeRingDelta){
    lightLargeLedRing(0, gLargeRingCounter, gLargeRingColor);
    gLargeRingTime = millis();
    gLargeRingCounter += gLargeRingLedStep;
    gLargeRingColor += gLargeRingColorStep * gLargeRingLedStep;
  }

  if (gSmallRingCounter > NUM_PIXELS_SMALL) {
    clearSmallLedRing();
    gSmallRingCounter = gSmallRingLedStep;
    gSmallRingColor = 0;
  }

  if (gLargeRingCounter > NUM_PIXELS_LARGE) {
    clearLargeLedRing();
    gLargeRingCounter = gLargeRingLedStep;
    gLargeRingColor = 0;
  }
}

void setSwitch() {
  if (digitalRead(SWITCH_PIN) == HIGH) {
    gSwitchDisplay = "X";
  } else {
    gSwitchDisplay = " ";
  }
}

void setButton() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    gButtonDisplay = " ";
  } else {
    gButtonDisplay = "X";
  }
}

void setBPM() {
  int potRead = analogRead(LEFT_POT_PIN);
  // Change value only if current read is different than previous set value
  if (abs(gLeftPotVal - potRead) > 20) {
    gBpm = map(potRead, 0, 1023, 10, 300);
    gDelayTime = 60.0 / gBpm * 1000.0;
    gDelayTimeInt = gDelayTime;
    gLargeRingDelta = gDelayTime / gLargeRingMultiplier;
    gSmallRingDelta = gDelayTime / gSmallRingMultiplier;
    gDisplayDelta = gDelayTime / gDisplayMultiplier;
  }
}

void setColor() {
  int potRead = analogRead(RIGHT_POT_PIN);
  // Change value only if current read is different than previous set value
  if (abs(gRightPotVal - potRead) > 20) {
    gSmallRingColor = map(potRead, 0, 1023, 0, 255);
    gLargeRingColor = map(potRead, 0, 1023, 0, 255);
    gDisplayMultiplier = map(potRead, 0, 1023, 1, 8);
    gSmallRingMultiplier = map(potRead, 0, 1023, 1, 8);
    gLargeRingMultiplier = map(potRead, 0, 1023, 1, 8);
  }
}

void printDisplay(){
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(" BPM  CLR");
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println(gBpm);
  display.setCursor(75, 20);
  display.println(gSmallRingColor);
  display.setCursor(0, 40);
  display.setTextSize(1);
  int beatNum = ceil(gCurrentTime - gDisplayTime) / gDisplayDelta;
  if (beatNum == 0){
    display.println(">                   <");
  } else if (beatNum == 1) {
    display.println("> >               < <");
  } else if (beatNum == 2) {
    display.println("> > >           < < <");
  } else if (beatNum == 3) {
    display.println("> > > >   .   < < < <");
  } else if (beatNum > 3) {
    gDisplayTime = millis();
  }
  display.setCursor(0, 50);
  display.println("MULT: ");
  display.setCursor(40, 50);
  display.println(gDisplayMultiplier);
  display.setCursor(50, 50);
  display.println("x    |");
  display.setCursor(90, 50);
  display.println(gButtonDisplay);
  display.setCursor(100, 50);
  display.println("|");
  display.setCursor(110, 50);
  display.println(gSwitchDisplay);
  display.setCursor(120, 50);
  display.println("|");
  // display.setCursor(120, 50);
  display.display();
}

// ----------------------------------------------------------------------------------
// LED RING FUNCTIONS
// ----------------------------------------------------------------------------------
void clearLargeLedRing() {
  for(int i = 0; i<NUM_PIXELS_LARGE; i++){
    gLargeLedRing.setPixelColor(i, 0x000000);
    gLargeLedRing.show();
  }
}

void lightLargeLedRing(int startIndex, int nPixels, int colorIdx) {
  for(int i=startIndex;i<startIndex+nPixels;i++){
    gLargeLedRing.setPixelColor(i, colorWheel(colorIdx));
    gLargeLedRing.show();
  }
}

uint32_t colorWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return gLargeLedRing.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return gLargeLedRing.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return gLargeLedRing.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
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
      gLargeLedRing.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        gLargeLedRing.setPixelColor(x-1, old_val[x-1]);
      }
      gLargeLedRing.show();
      delay(speed);
    }
   for (int count = NUM_PIXELS_LARGE-1; count>=0; count--) {
     gLargeLedRing.setPixelColor(count, color);
     old_val[count] = color;
     for(int x = count; x<=NUM_PIXELS_LARGE ;x++) {
       old_val[x-1] = dimColor(old_val[x-1], width);
       gLargeLedRing.setPixelColor(x+1, old_val[x+1]);
     }
     gLargeLedRing.show();
     delay(speed);
   }
  }
  if (clearAll){
    void clearLargeLedRing();
  }
}

// ----------------------------------------------------------------------------------
// LED RING FUNCTIONS
// ----------------------------------------------------------------------------------
void clearSmallLedRing() {
  for(int i = 0; i<NUM_PIXELS_SMALL; i++){
    gSmallLedRing.setPixelColor(i, 0x000000);
    gSmallLedRing.show();
  }
}

void lightSmallLedRing(int startIndex, int nPixels, int colorIdx) {
  for(int i=startIndex;i<startIndex+nPixels;i++){
    gSmallLedRing.setPixelColor(i, colorWheel(colorIdx));
    gSmallLedRing.show();
  }
}

// LED Animation (KnightRider)
void ledAnim2(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color, bool clearAll) {
  uint32_t old_val[NUM_PIXELS_SMALL]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 0; count<NUM_PIXELS_SMALL + 1; count++) {
      gSmallLedRing.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        gSmallLedRing.setPixelColor(x-1, old_val[x-1]);
      }
      gSmallLedRing.show();
      delay(speed);
    }
   for (int count = NUM_PIXELS_SMALL-1; count>=0; count--) {
     gSmallLedRing.setPixelColor(count, color);
     old_val[count] = color;
     for(int x = count; x<=NUM_PIXELS_SMALL ;x++) {
       old_val[x-1] = dimColor(old_val[x-1], width);
       gSmallLedRing.setPixelColor(x+1, old_val[x+1]);
     }
     gSmallLedRing.show();
     delay(speed);
   }
  }
  if (clearAll){
    void clearSmallLedRing();
  }
}
