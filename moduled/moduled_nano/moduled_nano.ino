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

// CROSS BIG TO SMALL ANIMATION
const uint64_t ANIM1[] PROGMEM = {
  0xffc3a59999a5c3ff,
  0x8142241818244281,
  0x0042241818244200,
  0x0000241818240000,
  0x0000001818000000,
  0x0000000810000000,
  0x0000001008000000,
  0x0000001818000000,
  0x0000241818240000,
  0x0042241818244200,
  0x8142241818244281,
  0xffc3a59999a5c3ff
};

// FILLING CIRCLES
const uint64_t ANIM2[] PROGMEM = {
  0x0000001818000000,
  0x0000182424180000,
  0x003c424242423c00,
  0x7e8181818181817e,
  0x7e8181999981817e,
  0x7e8181999981817e,
  0x7e8199a5a599817e,
  0x7ebdc3c3c3c3bd7e,
  0x7ebdc3dbdbc3bd7e,
  0x7ebddbe7e7dbbd7e,
  0x7ebddbffffdbbd7e
};

// const long ANIMATIONS[2][15] PROGMEM = {ANIM1, ANIM2};
int animIndex = 0;
int animSize = sizeof(ANIM1);
uint64_t matrixImage;
// uint64_t ANIMATION[];

// const int IMAGES_LEN = sizeof(IMAGES)/8;


// SETUP ---------------------------------------------------
void setup() {
  MIDI.begin(15);
  MIDI.setHandleNoteOn(MyHandleNoteOn);
  MIDI.setHandleNoteOff(MyHandleNoteOff);

  ledMatrix.clearDisplay(0);
  ledMatrix.shutdown(0, false);
  ledMatrix.setIntensity(0, 10);

  ledRing.begin(); // This initializes the NeoPixel library.
  ledRing.setBrightness(50);  // btw 0 - 127

  pinMode(switchPin, INPUT);
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
}
// -------------------------------------------------------
// LOOP -----------------------------------------------
int ledCounter = 0;
void loop() {

  if (digitalRead(switchPin) == HIGH) {
    // Read MIDI
    MIDI.read();
  } else {
    setAnimation();
    playAnimation();
    animIndex += 1;
    if (animIndex > 1) {
      animIndex = 0;
    }
  }

  setKnob1();
  setKnob2();

}
// -------------------------------------------------------

// MIDI FUNCTIONS -----------------------------------------------
void MyHandleNoteOn(byte channel, byte pitch, byte velocity) {
  uint64_t ANIMATION[animSize] = {setAnimation()};
  int IMAGES_LEN = sizeof(ANIMATION)/8;
  int imgIdx = map(pitch, 0, 127, 0, IMAGES_LEN - 1);
  uint64_t matrixImage;
  memcpy_P(&matrixImage, &ANIMATION[imgIdx], 8);
  displayImage(matrixImage);
}

void MyHandleNoteOff(byte channel, byte pitch, byte velocity) {
  ledMatrix.clearDisplay(0);
}
// KNOB FUNCTIONS -----------------------------------------------
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

// --------------------------------------------------------------
// LED MATRIX FUNCTIONS -----------------------------------------
void displayImage(uint64_t image) {
  for (int i = 0; i < 8; i++) {
    byte row = (image >> i * 8) & 0xFF;
    for (int j = 0; j < 8; j++) {
      ledMatrix.setLed(0, i, j, bitRead(row, j));
    }
  }
}

uint64_t setAnimation() {
   switch(animIndex){
      case 0: animSize = sizeof(ANIM1); return ANIM1; break;
      case 1: animSize = sizeof(ANIM2); return ANIM2; break;
  }
}

void playAnimation() {
  uint64_t ANIMATION[animSize] = {setAnimation()};
  int IMAGES_LEN = sizeof(ANIMATION)/8;
  for(int imgIdx = 0; imgIdx<IMAGES_LEN; imgIdx++){
    uint64_t matrixImage;
    memcpy_P(&matrixImage, &ANIMATION[imgIdx], 8);
    displayImage(matrixImage);
    delay(200);
    setKnob1();
    setKnob2();
//    int ledStartIndex = 0;
//    if (ledCounter > ledRingPixels) {
//      ledCounter = 1;
//      clearLedRing();
//      // ledStartIndex = 1;
//    }
//    lightLedRing(ledStartIndex, ledCounter, colorIdx);
//    ledCounter += 1;
    ledAnim(1, 50, 8, colorWheel(colorIdx), true);
  }
}
// ------------------------------------------------------------
// LED RING FUNCTIONS ----------------------------------------
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
// -----------------------------------------------------------
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
