/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Large LED Ring
const byte LED_RING1_PIN = 6;
const byte NUM_PIXELS1 = 24;
Adafruit_NeoPixel ledRing1 = Adafruit_NeoPixel(NUM_PIXELS1, LED_RING1_PIN, NEO_GRB + NEO_KHZ800);

// Small LED Ring
const byte LED_RING2_PIN = 5;
const byte NUM_PIXELS2 = 12;
Adafruit_NeoPixel ledRing2 = Adafruit_NeoPixel(NUM_PIXELS2, LED_RING2_PIN, NEO_GRB + NEO_KHZ800);


byte gColorIdx = 0;
byte gColorStep = 9;
byte gLedStep = 3;

// KNOBS
// KNOB 1 - Led Ring Step Size
// KNOB 2 - Led Ring Color Step Size
// KNOB 3 - BPM and multiplier
// SWITCH
// Between state 1 and 2

const byte POT_PIN = 3;
const byte LED_PIN = 9;
int bpm = 120;
int gPotVal = 500;
float delayTime = 60.0 / bpm * 1000.0;
int delayTimeInt = delayTime;
byte ledCounter = 0;
unsigned long currentTime;
unsigned long timeDelta1 = delayTime;
unsigned long timeDelta2 = delayTime;


void setup() {
  pinMode(LED_PIN, OUTPUT);

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
  display.setTextColor(WHITE);
  display.setCursor(20, 20);
  // Display static text
  display.println("MODULED!");
  display.display();
  delay(1000);
}

void loop() {
  setBPM();
  printDisplay();
  
  // digitalWrite(LED_PIN, HIGH);
  // delay(delayTime);
  if (millis() - currentTime > timeDelta1){
    lightLedRing1(0, ledCounter, gColorIdx);
    ledCounter += gLedStep;
    gColorIdx += gColorStep * gLedStep;
    currentTime = millis();
  }
  if (millis() - currentTime > timeDelta2){
    lightLedRing2(0, ledCounter, gColorIdx);
    ledCounter += gLedStep;
    gColorIdx += gColorStep * gLedStep;
  }
  // digitalWrite(LED_PIN, LOW);
  
  
  if (ledCounter > NUM_PIXELS1) {
    clearLedRing1();
    clearLedRing2();
    ledCounter = gLedStep;
    gColorIdx = 0;
  }
}

void setBPM() {
  int potRead = analogRead(POT_PIN);
  // Change value only if current read is different than previous set value
  if (abs(gPotVal - potRead) > 20) {
    bpm = map(potRead, 0, 1023, 10, 300);
    delayTime = 60.0 / bpm * 1000.0;
    delayTimeInt = delayTime;
  }
}

void printDisplay(){
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(" BPM | DLY");
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println(bpm);
  display.setCursor(85, 20);
  display.println(delayTimeInt);
  display.setCursor(0, 40);
  display.setTextSize(1);
  display.println("> > > > - - - < < < <");
  display.display();
}

// ----------------------------------------------------------------------------------
// LED RING FUNCTIONS
// ----------------------------------------------------------------------------------
void clearLedRing1() {
  for(int i = 0; i<NUM_PIXELS1; i++){
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
  uint32_t old_val[NUM_PIXELS1]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 0; count<NUM_PIXELS1 + 1; count++) {
      ledRing1.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        ledRing1.setPixelColor(x-1, old_val[x-1]);
      }
      ledRing1.show();
      delay(speed);
    }
   for (int count = NUM_PIXELS1-1; count>=0; count--) {
     ledRing1.setPixelColor(count, color);
     old_val[count] = color;
     for(int x = count; x<=NUM_PIXELS1 ;x++) {
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
  for(int i = 0; i<NUM_PIXELS2; i++){
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
  uint32_t old_val[NUM_PIXELS2]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 0; count<NUM_PIXELS2 + 1; count++) {
      ledRing2.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        ledRing2.setPixelColor(x-1, old_val[x-1]);
      }
      ledRing2.show();
      delay(speed);
    }
   for (int count = NUM_PIXELS2-1; count>=0; count--) {
     ledRing2.setPixelColor(count, color);
     old_val[count] = color;
     for(int x = count; x<=NUM_PIXELS2 ;x++) {
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
