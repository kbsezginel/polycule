// ----------------------------------------------------------------------------------
// POLYCULE | MODULED | LED Animation with MIDI
// ----------------------------------------------------------------------------------
#include <MIDI.h>
#include <LedControl.h>
#include <Adafruit_NeoPixel.h>
// ---------------------------------- MIDI SETUP ------------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
// ---------------------------- NEOPIXEL LED RING SETUP -----------------------------
#define PIN            5
#define NUMPIXELS      8
Adafruit_NeoPixel ledRing = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int colorIdx = 0;
// ----------------------------- POTENTIOMETER SETUP --------------------------------
const int potPin1 = 4;
const int potPin2 = 6;
int intensityVal;
int delayVal = 100;
// -------------------------------- SWITCH SETUP ------------------------------------
const int switchPin = 2;
// -------------------------------- BUTTON SETUP ------------------------------------
const int buttonPin1 = 2;
const int buttonPin2 = 20;
// ------------------------------- LED MATRIX SETUP ---------------------------------
// pin 12 is connected to the DataIn (1)
// pin 11 is connected to the CLK  (2)
// pin 10 is connected to LOAD (3)
LedControl ledMatrix = LedControl(12,11,10,1);


// CROSS BIG TO SMALL ANIMATION
const uint64_t IMAGES[] PROGMEM = {
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


const int IMAGES_LEN = sizeof(IMAGES)/8;
uint64_t matrixImage;


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
}
// -------------------------------------------------------
// LOOP -----------------------------------------------
int i = 0;
void loop() {
  // Read MIDI
  MIDI.read();
  
//  lightLedRing(i, colorIdx);
//  colorIdx += 21;
//  delay(delayVal); 

  setLEDIntensity();
  setDelay();
}
// -------------------------------------------------------

// MIDI FUNCTIONS -----------------------------------------------
void MyHandleNoteOn(byte channel, byte pitch, byte velocity) {
  // int boxSize = map(velocity, 0, 127, 1, 4); 
  // lightAllLedMatrix();
  
  int imgBrightess = map(velocity, 0, 127, 0, 16);
  ledMatrix.setIntensity(0, imgBrightess);

  int imgIdx = map(pitch, 0, 127, 0, IMAGES_LEN - 1);
  uint64_t matrixImage;
  memcpy_P(&matrixImage, &IMAGES[imgIdx], 8);
  displayImage(matrixImage);
}

void MyHandleNoteOff(byte channel, byte pitch, byte velocity) { 
  ledMatrix.clearDisplay(0);
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

void setLEDIntensity(){
  int intensityRead = analogRead(potPin1);
  intensityVal = map(intensityRead, 0, 1023, 0, 16);
  ledMatrix.setIntensity(0, intensityVal);
  intensityVal = map(intensityRead, 0, 1023, 1, 127);
  ledRing.setBrightness(intensityVal);
}
// ------------------------------------------------------------
// LED RING FUNCTIONS ----------------------------------------
void clearLedRing() {
  for( int i = 0; i<NUMPIXELS; i++){
    ledRing.setPixelColor(i, 0x000000);
    ledRing.show();
  }
}

void lightLedRing(int nPixels, int colorIdx) {
    for(int i=0;i<nPixels;i++){
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
void setDelay(){
  int delayRead = analogRead(potPin2);
  delayVal = map(delayRead, 0, 1023, 0, 1000);
  if (delayVal > 50) {
    delay(delayVal);
  }
}
