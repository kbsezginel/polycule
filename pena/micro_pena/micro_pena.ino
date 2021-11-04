#include <MIDI.h>

// Rotary Encoder 1
#define RE1_CLK_PIN 2
#define RE1_DT_PIN 3
#define RE1_BUTTON_PIN 4
// Rotary Encoder 2
#define RE2_CLK_PIN 5
#define RE2_DT_PIN 6
#define RE2_BUTTON_PIN 7

#define SWITCH_PIN 8

// ------------------------------- SWITCH SETUP --------------------------------
bool setupMode = false;

// -------------------------------- MIDI SETUP ---------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
const byte MIDI_CHANNEL = 1;

// ---------------------------- ROTARY ENCODER SETUP ---------------------------
byte sensitivityRE = 7;

int posRE1 = 0;
int valRE1;
int lastValRE1;
byte sendRE1;          // Rotary endcoder 1 send value
byte sendMaxRE1 = 12;  // Upper limit for the send value
byte sendMinRE1 = 0;   // Lower limit for the send value
bool valChangeRE1 = false;


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

  // Switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // Setup rotary encoder 1 pins
  pinMode(RE1_CLK_PIN, INPUT_PULLUP);
  pinMode(RE1_DT_PIN, INPUT_PULLUP);
  pinMode(RE1_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RE1_CLK_PIN), readRE1, CHANGE);

  // Setup rotary encoder 2 pins
  pinMode(RE2_CLK_PIN, INPUT_PULLUP);
  pinMode(RE2_DT_PIN, INPUT_PULLUP);
  pinMode(RE2_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RE1_CLK_PIN), readRE2, CHANGE);

  Serial.begin(9600);
}
// #############################################################################
void loop() {


  if (digitalRead(RE1_BUTTON_PIN) == LOW){
   // Button pressed
    Serial.print("RE1 Pressed");
  }

  if (digitalRead(RE2_BUTTON_PIN) == LOW){
   // Button pressed
    Serial.print("RE2 Pressed");
  }

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

void readRE1() {
  if (digitalRead(RE1_CLK_PIN) == digitalRead(RE1_DT_PIN)) {
    posRE1--;
  } else {
    posRE1++;
  }
  valRE1 = posRE1 / sensitivityRE;

  if (valRE1 > lastValRE1) {
    sendRE1++;
    sendRE1 = min(sendRE1, sendMaxRE1);
    valChangeRE1 = true;
  }
  else if(valRE1 < lastValRE1) {
    sendRE1--;
    sendRE1 = max(sendMinRE1, sendRE1);
    valChangeRE1 = true;
  } else {
    valChangeRE1 = false;
  }

  Serial.println("RE1 val " + sendRE1);
  lastValRE1 = valRE1;
}


void readRE2() {
  if (digitalRead(RE2_CLK_PIN) == digitalRead(RE2_DT_PIN)) {
    posRE2--;
  } else {
    posRE2++;
  }
  valRE2 = posRE2 / sensitivityRE;

  if (valRE2 > lastValRE2) {
    sendRE2++;
    sendRE2 = min(sendRE2, sendMaxRE2);
    valChangeRE2 = true;
  }
  else if(valRE2 < lastValRE2) {
    sendRE2--;
    sendRE2 = max(sendMinRE2, sendRE2);
    valChangeRE2 = true;
  } else {
    valChangeRE2 = false;
  }

  Serial.println("RE2 val " + sendRE2);
  lastValRE2 = valRE2;
}

// void readSwitch() {
//    if (digitalRead(SWITCH_PIN) == LOW){
//       // UP
//       if (valChange) {
//         clearLedRing();
//       }
//       lightLedRing(0, valPot, valRotary);
//       setupMode = true;
// //      ledRing.setPixelColor(1, ledRing.Color(10, 150, 50));
// //      ledRing.setPixelColor(2, ledRing.Color(10, 150, 70));
// //      ledRing.setPixelColor(3, ledRing.Color(10, 150, 90));
//       ledRing.show();
//    } else {
//      // DOWN
//      setupMode = false;
//       // ledRing.setPixelColor(4, ledRing.Color(100, 10, 10));
//       // TODO: clear only when changed
//       // clearLedRing();
//       colorLedRing(0, keyPadButton, ledRing.Color(100, 10, 10));
//       ledRing.show();
//    }
// }
