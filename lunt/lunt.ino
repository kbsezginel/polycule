// ----------------------------------------------------------------------------------
// POLYCULE | LUNT | Light Bulb Animation
// ----------------------------------------------------------------------------------
#include <MIDI.h>
#include "dimmable_light.h"
// ---------------------------------- MIDI SETUP ------------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 15
// -------------------------------- DIMMMER SETUP -----------------------------------
#define DIM_SYNC_PIN 2
#define DIM1_PIN 11
#define DIM2_PIN 12
#define DIM3_PIN 9
#define DIM4_PIN 10
DimmableLight dimLight1(DIM1_PIN);
DimmableLight dimLight2(DIM2_PIN);
DimmableLight dimLight3(DIM3_PIN);
DimmableLight dimLight4(DIM4_PIN);
// ----------------------------------------------------------------------------------

void setup() {
  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(MyHandleNoteOn);
  MIDI.setHandleNoteOff(MyHandleNoteOff);
  MIDI.setHandleControlChange(MyCCFunction);

  DimmableLight::setSyncPin(DIM_SYNC_PIN);
  DimmableLight::begin();
}

void loop() {
  MIDI.read();
}

// ----------------------------------------------------------------------------------
// MIDI FUNCTIONS
// ----------------------------------------------------------------------------------
void MyHandleNoteOn(byte channel, byte pitch, byte velocity) {
  dimLight1.setBrightness(250);
  dimLight2.setBrightness(250);
  dimLight3.setBrightness(250);
  dimLight4.setBrightness(250);
}

void MyHandleNoteOff(byte channel, byte pitch, byte velocity) {
  dimLight1.setBrightness(70);
  dimLight2.setBrightness(70);
  dimLight3.setBrightness(70);
  dimLight4.setBrightness(70);
}

void MyCCFunction(byte channel, byte number, byte value) {
  int brightness = 80;
  switch (number) {
    case 22:
      brightness = map(value, 0, 127, 60, 255);
      dimLight1.setBrightness(brightness);
      break;
    case 23:
      brightness = map(value, 0, 127, 60, 255);
      dimLight2.setBrightness(brightness);
      break;
    case 24:
      brightness = map(value, 0, 127, 60, 255);
      dimLight3.setBrightness(brightness);
      break;
    case 25:
      brightness = map(value, 0, 127, 60, 255);
      dimLight4.setBrightness(brightness);
      break;
  }
}
