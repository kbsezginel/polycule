// ----------------------------------------------------------------------------------
// POLYCULE | MODULED | LED Animation with MIDI
// ----------------------------------------------------------------------------------
#include <MIDI.h>
#include <LedControl.h>
#include <Adafruit_NeoPixel.h>
// ---------------------------------- MIDI SETUP ------------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
// ---------------------------- NEOPIXEL LED RING SETUP -----------------------------
const int ledRingPin = 13;
const int ledRingPixels = 8;
Adafruit_NeoPixel ledRing = Adafruit_NeoPixel(ledRingPixels, ledRingPin, NEO_GRB + NEO_KHZ800);
int colorIdx = 0;
// ----------------------------- POTENTIOMETER SETUP --------------------------------
const int potPin1 = 4;
const int potPin2 = 6;
int intensityVal;
int delayVal = 100;
// -------------------------------- SWITCH SETUP ------------------------------------
const int switchPin = 6;
// -------------------------------- BUTTON SETUP ------------------------------------
const int buttonPin1 = 4;
const int buttonPin2 = 8;
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

const int NUMFRAMES[] = {12, 11, 16, 28};
const int ANIMSTART[] = {0, 12, 23, 39};
const int NUMANIMATIONS = 4;
int gAnimIndex = 0;
// ----------------------------------------------------------------------------------
// >x< SETUP >x<
// ----------------------------------------------------------------------------------
void setup() {
  MIDI.begin(15);
  MIDI.setHandleNoteOn(MyHandleNoteOn);
  MIDI.setHandleNoteOff(MyHandleNoteOff);

  ledMatrix.clearDisplay(0);
  ledMatrix.shutdown(0, false);
  ledMatrix.setIntensity(0, 10);

  ledRing.begin();
  ledRing.setBrightness(50);  // btw 0 - 127

  pinMode(switchPin, INPUT);
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
}
// ----------------------------------------------------------------------------------
// oOo LOOP oOo
// ----------------------------------------------------------------------------------
int ledCounter = 0;
void loop() {
  if (digitalRead(switchPin) == HIGH) {
    // Read MIDI
    MIDI.read();
  } else {
    playAnimation();
  }
  setAnimationIndex();
  setKnob1();
  setKnob2();
}
// ----------------------------------------------------------------------------------
// MIDI FUNCTIONS
// ----------------------------------------------------------------------------------
void MyHandleNoteOn(byte channel, byte pitch, byte velocity) {
  int imgIdx = map(pitch, 0, 127, ANIMSTART[gAnimIndex], NUMFRAMES[gAnimIndex] - 1);
  displayImage(ANIMATIONS[imgIdx]);
}

void MyHandleNoteOff(byte channel, byte pitch, byte velocity) {
  ledMatrix.clearDisplay(0);
}
// ----------------------------------------------------------------------------------
// KNOB FUNCTIONS
// ----------------------------------------------------------------------------------
void setKnob1() {
  // Sets LED brightness
  int potRead = analogRead(potPin1);
  intensityVal = map(potRead, 0, 1023, 0, 16);
  ledMatrix.setIntensity(0, intensityVal);
  intensityVal = map(potRead, 0, 1023, 1, 127);
  ledRing.setBrightness(intensityVal);
}

void setKnob2() {
  // Sets LED Ring Color
  int potRead = analogRead(potPin2);
  colorIdx = map(potRead, 0, 1023, 0, 232);
}

bool setAnimationIndex() {
  bool animChange = false;
  if (digitalRead(buttonPin1) == HIGH){
    gAnimIndex -= 1;
    animChange = true;
  }
  if (digitalRead(buttonPin2) == HIGH){
    gAnimIndex += 1;
    animChange = true;
  }
  if (gAnimIndex > NUMANIMATIONS - 1 || gAnimIndex < 0){
    gAnimIndex = 0;
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
  for(int imgIdx = ANIMSTART[gAnimIndex]; imgIdx<ANIMSTART[gAnimIndex] + NUMFRAMES[gAnimIndex]; imgIdx++){
    displayImage(ANIMATIONS[imgIdx]);
    delay(100);
    setKnob1();
    setKnob2();
    if (setAnimationIndex()) {
      break;
    }
    
//    int ledStartIndex = 0;
//    if (ledCounter > ledRingPixels) {
//      ledCounter = 1;
//      clearLedRing();
//      // ledStartIndex = 1;
//    }
//    lightLedRing(ledStartIndex, ledCounter, colorIdx);
//    ledCounter += 1;
    // ledAnim(1, 50, 8, colorWheel(colorIdx), true);
  }
}
// ----------------------------------------------------------------------------------
// LED RING FUNCTIONS ---------------------------------------------------------------
void clearLedRing() {
  for( int i = 0; i<ledRingPixels; i++){
    ledRing.setPixelColor(i, 0x000000);
    ledRing.show();
  }
}

void lightLedRing(int startIndex, int nPixels, int colorIdx) {
    for(int i=startIndex;i<startIndex+nPixels;i++){
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    // pixels.setPixelColor(i, pixels.Color(0,150,0)); // Moderately bright green color.
    ledRing.setPixelColor(i, colorWheel(colorIdx));
    ledRing.show(); // This sends the updated pixel color to the hardware.
  }
}

uint32_t colorWheel(byte WheelPos) {
  byte state = WheelPos / 21;
  switch(state) {
    case 0: return ledRing.Color(255, 0, 255 - ((((WheelPos % 21) + 1) * 6) + 127)); break;
    case 1: return ledRing.Color(255, ((WheelPos % 21) + 1) * 6, 0); break;
    case 2: return ledRing.Color(255, (((WheelPos % 21) + 1) * 6) + 127, 0); break;
    case 3: return ledRing.Color(255 - (((WheelPos % 21) + 1) * 6), 255, 0); break;
    case 4: return ledRing.Color(255 - (((WheelPos % 21) + 1) * 6) + 127, 255, 0); break;
    case 5: return ledRing.Color(0, 255, ((WheelPos % 21) + 1) * 6); break;
    case 6: return ledRing.Color(0, 255, (((WheelPos % 21) + 1) * 6) + 127); break;
    case 7: return ledRing.Color(0, 255 - (((WheelPos % 21) + 1) * 6), 255); break;
    case 8: return ledRing.Color(0, 255 - ((((WheelPos % 21) + 1) * 6) + 127), 255); break;
    case 9: return ledRing.Color(((WheelPos % 21) + 1) * 6, 0, 255); break;
    case 10: return ledRing.Color((((WheelPos % 21) + 1) * 6) + 127, 0, 255); break;
    case 11: return ledRing.Color(255, 0, 255 - (((WheelPos % 21) + 1) * 6)); break;
    default: return ledRing.Color(0, 0, 0); break;
  }
}
// ----------------------------------------------------------------------------------
// LED Color Dim
// ----------------------------------------------------------------------------------
uint32_t dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}
// ----------------------------------------------------------------------------------
// LED Animation (KnightRider)
// ----------------------------------------------------------------------------------
void ledAnim(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color, bool clearAll) {
  uint32_t old_val[ledRingPixels]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 0; count<ledRingPixels + 1; count++) {
      ledRing.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        ledRing.setPixelColor(x-1, old_val[x-1]);
      }
      ledRing.show();
      delay(speed);
    }
//    for (int count = ledRingPixels-1; count>=0; count--) {
//      ledRing.setPixelColor(count, color);
//      old_val[count] = color;
//      for(int x = count; x<=ledRingPixels ;x++) {
//        old_val[x-1] = dimColor(old_val[x-1], width);
//        ledRing.setPixelColor(x+1, old_val[x+1]);
//      }
//      ledRing.show();
//      delay(speed);
//    }
  }
  if (clearAll){
    void clearLedRing();
  }
}
