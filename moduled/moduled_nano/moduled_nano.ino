// ----------------------------------------------------------------------------------
// POLYCULE | MODULED | LED Animation with MIDI
// ----------------------------------------------------------------------------------
#include <MIDI.h>
#include <LedControl.h>
#include <Adafruit_NeoPixel.h>
// ---------------------------------- MIDI SETUP ------------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
const byte MIDI_CHANNEL = 15;
// ---------------------------- NEOPIXEL LED RING SETUP -----------------------------
const byte LED_RING_PIN = 13;
const byte NUM_PIXELS = 8;
Adafruit_NeoPixel ledRing = Adafruit_NeoPixel(NUM_PIXELS, LED_RING_PIN, NEO_GRB + NEO_KHZ800);
byte gColorIdx = 0;
// ----------------------------- POTENTIOMETER SETUP --------------------------------
const byte POT_PIN_1 = 6;
const byte POT_PIN_2 = 4;
int gPotVal1 = 500;
int gPotVal2 = 500;
// -------------------------------- SWITCH SETUP ------------------------------------
const byte SWITCH_PIN = 6;
// -------------------------------- BUTTON SETUP ------------------------------------
const byte BUTTON_PIN_1 = 4;
const byte BUTTON_PIN_2 = 8;
const byte BUTTON_DELAY = 250;  // Wait in between button presses to avoid jumping
// ------------------------------- LED MATRIX SETUP ---------------------------------
// 1) DIN (Pin 12) | 2) CLK (PIN 11) | 3) LOAD or CS (PIN 10)
LedControl ledMatrix = LedControl(12,11,10,1);
// ----------------------------------------------------------------------------------
// ~ ~ ~ ANIMATIONS ~ ~ ~
// ----------------------------------------------------------------------------------
const uint64_t ANIMATIONS[] = {
  0xffc3a59999a5c3ff,  // ANIM 1 | Start: 0 | Frames: 12
  0x8142241818244281,  // CROSS BIG TO SMALL ANIMATION
  0x0042241818244200,
  0x0000241818240000,
  0x0000001818000000,
  0x0000000810000000,
  0x0000001008000000,
  0x0000001818000000,
  0x0000241818240000,
  0x0042241818244200,
  0x8142241818244281,
  0xffc3a59999a5c3ff,
  0x0000001818000000,  // ANIM 2 | Start: 12 | Frames: 11
  0x0000182424180000,  // FILLING CIRCLES
  0x003c424242423c00,
  0x7e8181818181817e,
  0x7e8181999981817e,
  0x7e8181999981817e,
  0x7e8199a5a599817e,
  0x7ebdc3c3c3c3bd7e,
  0x7ebdc3dbdbc3bd7e,
  0x7ebddbe7e7dbbd7e,
  0x7ebddbffffdbbd7e,
  0x0000001818000000,  // ANIM 3 | Start: 23 | Frames: 16
  0x0000182424180000,  // PHARMACY
  0x0018186666181800,
  0x181818e7e7181818,
  0x991818e7e7181899,
  0xdbdb18e7e718dbdb,
  0xffffffe7e7ffffff,
  0xffffffffffffffff,
  0xffffffffffffffff,
  0xffffffe7e7ffffff,
  0xdbdb18e7e718dbdb,
  0x991818e7e7181899,
  0x181818e7e7181818,
  0x0018186666181800,
  0x0000182424180000,
  0x0000001818000000,
  0x0100000000000000,  // ANIM 4 | Start: 39 | Frames: 28
  0x0101000000000000,  // CIRCLING BOX
  0x0101010000000000,
  0x0101010100000000,
  0x0101010101000000,
  0x0101010101010000,
  0x0101010101010100,
  0x0101010101010101,
  0x0101010101010103,
  0x0101010101010107,
  0x010101010101010f,
  0x010101010101011f,
  0x010101010101013f,
  0x010101010101017f,
  0x01010101010101ff,
  0x01010101010181ff,
  0x01010101018181ff,
  0x01010101818181ff,
  0x01010181818181ff,
  0x01018181818181ff,
  0x01818181818181ff,
  0x81818181818181ff,
  0xc1818181818181ff,
  0xe1818181818181ff,
  0xf1818181818181ff,
  0xf9818181818181ff,
  0xfd818181818181ff,
  0xff818181818181ff
};

const int NUM_FRAMES[] = {12, 11, 16, 28};
const int ANIM_START[] = {0, 12, 23, 39};
const byte NUM_ANIMATIONS = 4;
int gAnimDelay = 100;
int gAnimIndex = 0;
bool gMidiMode = false;
// ----------------------------------------------------------------------------------
// >x< SETUP >x<
// ----------------------------------------------------------------------------------
void setup() {
  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(MyHandleNoteOn);
  MIDI.setHandleNoteOff(MyHandleNoteOff);
  MIDI.setHandleControlChange(MyCCFunction);

  ledMatrix.clearDisplay(0);
  ledMatrix.shutdown(0, false);
  ledMatrix.setIntensity(0, 10);

  ledRing.begin();
  ledRing.setBrightness(50);  // btw 0 - 127

  pinMode(SWITCH_PIN, INPUT);
  pinMode(BUTTON_PIN_1, INPUT);
  pinMode(BUTTON_PIN_2, INPUT);

  ledAnim(3, 50, 8, colorWheel(gColorIdx), true);
}
// ----------------------------------------------------------------------------------
// oOo LOOP oOo
// ----------------------------------------------------------------------------------
void loop() {
  if (digitalRead(SWITCH_PIN) == HIGH) {
    gMidiMode = true;
    MIDI.read();
    setKnob1();
    setKnob2();
  } else {
    gMidiMode = false;
    playAnimation();
  }
  setAnimationIndex();
}
// ----------------------------------------------------------------------------------
// MIDI FUNCTIONS
// ----------------------------------------------------------------------------------
void MyHandleNoteOn(byte channel, byte pitch, byte velocity) {
  int imgIdx = map(pitch, 0, 127, ANIM_START[gAnimIndex], NUM_FRAMES[gAnimIndex] - 1);
  displayImage(ANIMATIONS[imgIdx]);
  lightLedRing(0, map(pitch, 0, 127, 0, NUM_PIXELS), gColorIdx);
}

void MyHandleNoteOff(byte channel, byte pitch, byte velocity) {
  ledMatrix.clearDisplay(0);
  clearLedRing();
}

void MyCCFunction(byte channel, byte number, byte value) {
  switch (number) {
    case 22:
      gAnimIndex = map(value, 0, 127, 0, NUM_ANIMATIONS - 1);
      break;
    case 23:
      ledMatrix.setIntensity(0, map(value, 0, 127, 0, 16));
      break;
    case 24:
      ledRing.setBrightness(value);
      break;
    case 25:
      gColorIdx = value * 2;
      break;
  }
}
// ----------------------------------------------------------------------------------
// KNOB FUNCTIONS
// ----------------------------------------------------------------------------------
void setKnob1() {
  // Sets LED brightness or Anim speed
  int potRead = analogRead(POT_PIN_1);
  // Change value only if current read is different than previous set value
  if (abs(gPotVal1 - potRead) > 10) {
    if (gMidiMode) {
      byte intensityVal = map(potRead, 0, 1023, 0, 16);
      ledMatrix.setIntensity(0, intensityVal);
      intensityVal = map(potRead, 0, 1023, 1, 127);
      ledRing.setBrightness(intensityVal);
    } else {
      gAnimDelay = map(potRead, 0, 1023, 10, 1000);
    }
  }
}

void setKnob2() {
  // Sets LED Ring Color
  int potRead = analogRead(POT_PIN_2);
  // Change value only if current read is different than previous set value
  if (abs(gPotVal2 - potRead) > 5) {
    gColorIdx = map(potRead, 0, 1023, 0, 255);
  }
}

bool setAnimationIndex() {
  bool animChange = false;
  if (digitalRead(BUTTON_PIN_1) == HIGH){
    gAnimIndex -= 1;
    animChange = true;
    delay(BUTTON_DELAY);
  }
  if (digitalRead(BUTTON_PIN_2) == HIGH){
    gAnimIndex += 1;
    animChange = true;
    delay(BUTTON_DELAY);
  }
  if (gAnimIndex > NUM_ANIMATIONS - 1) {
    gAnimIndex = 0;
  } else if (gAnimIndex < 0) {
    gAnimIndex = NUM_ANIMATIONS - 1;
  }
  return animChange;
}
// ----------------------------------------------------------------------------------
// LED MATRIX FUNCTIONS
// ----------------------------------------------------------------------------------
void displayImage(uint64_t image) {
  for (int i = 0; i < 8; i++) {
    byte row = (image >> i * 8) & 0xFF;
    for (int j = 0; j < 8; j++) {
      ledMatrix.setLed(0, i, j, bitRead(row, j));
    }
  }
}

void playAnimation() {
  for(int imgIdx = ANIM_START[gAnimIndex]; imgIdx<ANIM_START[gAnimIndex] + NUM_FRAMES[gAnimIndex]; imgIdx++){
    displayImage(ANIMATIONS[imgIdx]);
    delay(gAnimDelay);
    setKnob1();
    setKnob2();
    if (setAnimationIndex()) {
      break;
    }
    lightLedRing(0, 8, gColorIdx);
    // ledAnim(1, 50, 8, colorWheel(gColorIdx), true);
  }
}
// ----------------------------------------------------------------------------------
// LED RING FUNCTIONS
// ----------------------------------------------------------------------------------
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

// LED Animation (KnightRider)
void ledAnim(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color, bool clearAll) {
  uint32_t old_val[NUM_PIXELS]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 0; count<NUM_PIXELS + 1; count++) {
      ledRing.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        ledRing.setPixelColor(x-1, old_val[x-1]);
      }
      ledRing.show();
      delay(speed);
    }
   for (int count = NUM_PIXELS-1; count>=0; count--) {
     ledRing.setPixelColor(count, color);
     old_val[count] = color;
     for(int x = count; x<=NUM_PIXELS ;x++) {
       old_val[x-1] = dimColor(old_val[x-1], width);
       ledRing.setPixelColor(x+1, old_val[x+1]);
     }
     ledRing.show();
     delay(speed);
   }
  }
  if (clearAll){
    void clearLedRing();
  }
}
