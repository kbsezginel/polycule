#include <Adafruit_NeoPixel.h>
#include <MIDI.h>

#define RE_CLK_PIN 2
#define RE_DT_PIN 3
#define RE_BUTTON_PIN 4
#define SWITCH_PIN 5
#define LED_PIN 6
#define KEYPAD_PIN A7

// ------------------------------- SWITCH SETUP --------------------------------
bool setupMode = false;

// -------------------------------- MIDI SETUP ---------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
const byte MIDI_CHANNEL = 1;

// ---------------------------- ROTARY ENCODER SETUP ---------------------------
int encoder0Pos = 0;
int valRotary;
int lastValRotary;
int valPot = 0;
bool valChange = false;

byte keyPadTolerance = 10;
byte keyPadButton;
byte keyPadOld;

int note = 36; // C3 (https://newt.phys.unsw.edu.au/jw/notes.html)
int velocity = 127;

// ------------------------------ LED RING SETUP -------------------------------
const byte NUM_PIXELS = 12;
Adafruit_NeoPixel ledRing = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// #############################################################################
void setup() {
  // MIDI
   MIDI.begin(MIDI_CHANNEL);
  // MIDI.setHandleNoteOn(handleNoteOn);
  // MIDI.setHandleNoteOff(handleNoteOff);

  // Start led ring
  ledRing.begin();
  ledRing.setBrightness(50); // 0 - 255
  ledRing.setPixelColor(10, ledRing.Color(0, 150, 0));
  ledRing.setPixelColor(8, ledRing.Color(0, 150, 0));
  ledRing.setPixelColor(6, ledRing.Color(0, 150, 0));
  ledRing.show();

  // Switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // Setup rotary encoder pins
  pinMode(RE_CLK_PIN, INPUT_PULLUP);
  pinMode(RE_DT_PIN, INPUT_PULLUP);
  pinMode(RE_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RE_CLK_PIN), readEncoder, CHANGE);

  Serial.begin(9600);
}
// #############################################################################
void loop() {
  readKeyPad();
  readSwitch();
  // Rotary encoder
  readREButton();

//  if (valRotary>lastValRotary)
//  {
//    valPot = min(valRotary, 127);
//    MIDI.sendControlChange(20, min(valRotary, 120), 1);
//    Serial.print("MINUS");
//  }
// if(valRotary<lastValRotary)
//  {
//    valPot = max(valRotary, 0);
//    MIDI.sendControlChange(20, max(valRotary, 0), 1);
//    Serial.print("PLUS");
//  }
//  lastValRotary = valRotary;
  // Serial.println(valRotary);
  // Serial.println(" ");
  // Serial.println(valPot);
}
// #############################################################################

void readEncoder() {
  if (digitalRead(RE_CLK_PIN) == digitalRead(RE_DT_PIN)) {
    encoder0Pos--;
  } else {
    encoder0Pos++;
  }
  valRotary = encoder0Pos / 7;
  // valPot = encoder0Pos / 5;

  if (valRotary>lastValRotary) {
    valPot++;
    valPot = min(valPot, 12);
    valChange = true;
  }
  else if(valRotary<lastValRotary) {
    valPot--;
    valPot = max(0, valPot);
    valChange = true;
  } else {
    valChange = false;
  }
  Serial.println(valPot);
  lastValRotary = valRotary;
}


void readKeyPad() {
  int sensorValue = analogRead(KEYPAD_PIN);

  if (sensorValue - keyPadTolerance <= 1023 && sensorValue + keyPadTolerance >= 1023) {
    keyPadButton = 1;
  } else if (sensorValue - keyPadTolerance <= 930 && sensorValue + keyPadTolerance >= 930) {
    keyPadButton = 2;
  } else if (sensorValue - keyPadTolerance <= 853 && sensorValue + keyPadTolerance >= 853) {
    keyPadButton = 3;
  } else if (sensorValue - keyPadTolerance <= 785 && sensorValue + keyPadTolerance >= 785) {
    keyPadButton = 4;
  } else if (sensorValue - keyPadTolerance <= 729 && sensorValue + keyPadTolerance >= 729) {
    keyPadButton = 5;
  } else if (sensorValue - keyPadTolerance <= 681 && sensorValue + keyPadTolerance >= 681) {
    keyPadButton = 6;
  } else if (sensorValue - keyPadTolerance <= 637 && sensorValue + keyPadTolerance >= 637) {
    keyPadButton = 7;
    note = 40;
  } else if (sensorValue - keyPadTolerance <= 600 && sensorValue + keyPadTolerance >= 600) {
    keyPadButton = 8;
    note = 43;
  } else if (sensorValue - keyPadTolerance <= 567 && sensorValue + keyPadTolerance >= 567) {
    keyPadButton = 9;
    note = 45;
  } else if (sensorValue - keyPadTolerance <= 536 && sensorValue + keyPadTolerance >= 536) {
    keyPadButton = 10;
    note = 47;
  } else if (sensorValue - keyPadTolerance <= 509 && sensorValue + keyPadTolerance >= 509) {
    keyPadButton = 11;
    note = 49;
  } else if (sensorValue - keyPadTolerance <= 485 && sensorValue + keyPadTolerance >= 485) {
    keyPadButton = 12;
    note = 50;
  } else {
    keyPadButton = 0;
  }


  if (keyPadOld != keyPadButton) {
    note = keyPadButton * 8 + 10;
    MIDI.sendNoteOn(note, velocity, 1);
    // delay(200);                // Wait for a second
    // MIDI.sendNoteOff(note, 0, MIDI_CHANNEL);     // Stop the note
    keyPadOld = keyPadButton;
  }
}

void readREButton() {
   if (digitalRead(RE_BUTTON_PIN) == LOW){
    // Button pressed
     clearLedRing();
   }
}


void readSwitch() {
   if (digitalRead(SWITCH_PIN) == LOW){
      // UP
      if (valChange) {
        clearLedRing();
      }
      lightLedRing(0, valPot, valRotary);
      setupMode = true;
//      ledRing.setPixelColor(1, ledRing.Color(10, 150, 50));
//      ledRing.setPixelColor(2, ledRing.Color(10, 150, 70));
//      ledRing.setPixelColor(3, ledRing.Color(10, 150, 90));
      ledRing.show();
   } else {
     // DOWN
     setupMode = false;
      // ledRing.setPixelColor(4, ledRing.Color(100, 10, 10));
      // TODO: clear only when changed
      // clearLedRing();
      colorLedRing(0, keyPadButton, ledRing.Color(100, 10, 10));
      ledRing.show();
   }
}


void setupValue() {
  switch (keyPadButton) {
     case 0:
        ledRing.setBrightness(valRotary + 10);
        ledRing.show();
        break;
     case 1:
        MIDI.sendControlChange(21, min(valRotary, 120), MIDI_CHANNEL); 
        break;
     case 2:
        MIDI.sendControlChange(22, min(valRotary, 120), MIDI_CHANNEL);  
        break;
  }
}


void clearLedRing() {
  for(int i = 0; i< NUM_PIXELS; i++){
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

void colorLedRing(int startIndex, int nPixels, uint32_t ledColor) {
  for(int i=startIndex;i<startIndex+nPixels;i++){
    ledRing.setPixelColor(i, ledColor);
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
