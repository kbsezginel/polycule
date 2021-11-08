#include <MIDI.h>

// Rotary Encoder 1
#define RE1_CLK_PIN 2
#define RE1_DT_PIN 5
#define RE1_BUTTON_PIN 4

// Rotary Encoder 2
#define RE2_CLK_PIN 3
#define RE2_DT_PIN 6
#define RE2_BUTTON_PIN 7

#define SWITCH_PIN 8

// ------------------------------- SWITCH SETUP --------------------------------
bool setupMode = false;

// -------------------------------- MIDI SETUP ---------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
const byte MIDI_CHANNEL = 7;
byte ccRE1 = 1;
byte ccRE2 = 2;
int note = 36; // C3 (https://newt.phys.unsw.edu.au/jw/notes.html)
int velocity = 127;

// ---------------------------- ROTARY ENCODER SETUP ---------------------------
int sensitivityRE = 3;

int posRE1 = 0;
int valRE1;
int lastValRE1;
int sendRE1 = 0;          // Rotary endcoder 1 send value
int lastsendRE1 = 0;
byte sendMaxRE1 = 127;  // Upper limit for the send value
byte sendMinRE1 = 0;   // Lower limit for the send value
bool valChangeRE1 = false;
bool pressedRE1 = false;

int posRE2 = 0;
int valRE2;
int lastValRE2;
int sendRE2 = 0;          // Rotary endcoder 1 send value
int lastsendRE2 = 0;
byte sendMaxRE2 = 127;  // Upper limit for the send value
byte sendMinRE2 = 0;   // Lower limit for the send value
bool valChangeRE2 = false;
bool pressedRE2 = false;
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
  attachInterrupt(digitalPinToInterrupt(RE2_CLK_PIN), readRE2, CHANGE);

  // Serial.begin(9600);
}
// #############################################################################
void loop() {

  if (digitalRead(RE1_BUTTON_PIN) == LOW){
   // Button pressed
    // Serial.print("Pressed 1\n");
    MIDI.sendNoteOn(note, velocity, MIDI_CHANNEL);
    pressedRE1 = true;
    delay(100);
  } else {
    if (pressedRE1) {
      MIDI.sendNoteOff(note, velocity, MIDI_CHANNEL);
      pressedRE1 = false;
    }
  }

  if (digitalRead(RE2_BUTTON_PIN) == LOW){
   // Button pressed
    // Serial.print("Pressed 2\n");
    MIDI.sendControlChange(3, 127, MIDI_CHANNEL);
    delay(100);
  }

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

  if (sendRE1 != lastsendRE1) {
    MIDI.sendControlChange(ccRE1, sendRE1, MIDI_CHANNEL);
  }

  // Serial.print("1 ");
  // Serial.println(sendRE1);
  // Serial.println(" ");
  lastValRE1 = valRE1;
  lastsendRE2 = sendRE2;
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

  if (sendRE2 != lastsendRE2) {
    MIDI.sendControlChange(ccRE2, sendRE2, MIDI_CHANNEL);
  }

  // Serial.print("2 ");
  // Serial.println(sendRE2);
  lastValRE2 = valRE2;
  lastsendRE2 = sendRE2;
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
