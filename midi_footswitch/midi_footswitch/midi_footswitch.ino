#include <MIDI.h>
#include <Adafruit_NeoPixel.h>

// MIDI Setup
MIDI_CREATE_DEFAULT_INSTANCE();
// NEOPIXEL Setup
byte NUM_PIXELS = 8;
Adafruit_NeoPixel neoPixel(NUM_PIXELS, 2, NEO_GRB + NEO_KHZ800);
// Global Variables
byte ledBrightness = 50;
byte tolerance = 4;
byte pressTol = 50;
byte colorIdx = 0;
int pressCount = 0;
bool noteOn = false;
bool note1On = false;
bool note2On = false;
bool note3On = false;
bool note4On = false;
bool note5On = false;
bool sendMidi = true;
// Toggle setup
byte toggleCount = 1;
byte toggleNumber = 3;
byte toggle1Count = 1;
byte toggle2Count = 1;
byte toggle3Count = 1;
byte toggle4Count = 1;
byte toggle5Count = 1;
int toggleCounts[] = {2, 2, 2, 2, 2};
int pixelNums[] = {7, 6, 5, 4, 3};
byte color1 = 0;
byte color2 = 200;
byte color3 = 80;
// Switch read values
int switch0 = 1014;
int switch1 = 905;
int switch2 = 813;
int switch3 = 724;
int switch4 = 626;
int switch5 = 500;
int switch6 = 321;
// Note Mode
byte noteMidiChannel = 8;
byte pitch1 = 52; // E3
byte pitch2 = 55; // G3
byte pitch3 = 59; // B3
byte pitch4 = 64; // E4
byte pitch5 = 71; // B4
// Trig Mode
byte pitch = 60;
byte trig1MidiChannel = 1;
byte trig2MidiChannel = 2;
byte trig3MidiChannel = 3;
byte trig4MidiChannel = 4;
byte trig5MidiChannel = 5;
// Play Mode


// -----------------------------------------------------------------------------
void setup() {
  pinMode(A0, INPUT_PULLUP);
  MIDI.begin(1);
  neoPixel.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  neoPixel.show();            // Turn OFF all pixels ASAP
  neoPixel.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}
// -----------------------------------------------------------------------------
void loop() {
  int sensorValue = analogRead(A0);

  if (sensorValue - tolerance < switch1 && sensorValue + tolerance > switch1) {
    // 1
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note1On) {
        switch (toggleCount) {
        // Trig Mode
        case 1:
          lightNeoPixel(7, 1, color1);
          MIDI.sendNoteOn(pitch, 127, trig1MidiChannel);
          break;
        // Note Mode
        case 2:
          lightNeoPixel(7, 1, color2);
          MIDI.sendNoteOn(pitch1, 127, noteMidiChannel);
          break;
        // Play Mode
        case 3:
          toggleCounts[0] += 1;
          if (toggleCounts[0] > 2) {
            toggleCounts[0] = 1;
          }
          switch (toggleCounts[0]) {
            case 1:
              MIDI.sendControlChange(94, 127, trig1MidiChannel);
              clearPixel(7);
              break;
            case 2:
              MIDI.sendControlChange(94, 0, trig1MidiChannel);
              lightNeoPixel(7, 1, color3);
              break;
          }
          noteOn = false;
          delay(500);
          break;
        }
        note1On = true;
      }
      delay(10);
    }
  } else if (sensorValue - tolerance < switch2 && sensorValue + tolerance > switch2) {
    // 2
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note2On) {
        // MIDI.sendNoteOn(pitch, 127, 2);
        switch (toggleCount) {
        // Trig Mode
        case 1:
          lightNeoPixel(6, 1, color1);
          MIDI.sendNoteOn(pitch, 127, trig2MidiChannel);
          break;
        // Note Mode
        case 2:
          lightNeoPixel(6, 1, color2);
          MIDI.sendNoteOn(pitch2, 127, noteMidiChannel);
          break;
        // Play Mode
        case 3:
          toggleCounts[1] += 1;
          if (toggleCounts[1] > 2) {
            toggleCounts[1] = 1;
          }
          switch (toggleCounts[1]) {
            case 1:
              MIDI.sendControlChange(94, 127, trig2MidiChannel);
              clearPixel(6);
              break;
            case 2:
              MIDI.sendControlChange(94, 0, trig2MidiChannel);
              lightNeoPixel(6, 1, color3);
              break;
          }
          noteOn = false;
          delay(500);
          break;
        }
        note2On = true;
      }
      delay(10);
    }
  } else if (sensorValue - tolerance < switch3 && sensorValue + tolerance > switch3) {
    // 3
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note3On) {
        switch (toggleCount) {
        // Trig Mode
        case 1:
          lightNeoPixel(5, 1, color1);
          MIDI.sendNoteOn(pitch, 127, trig3MidiChannel);
          break;
        // Note Mode
        case 2:
          lightNeoPixel(5, 1, color2);
          MIDI.sendNoteOn(pitch3, 127, noteMidiChannel);
          break;
        // Play Mode
        case 3:
          toggleCounts[2] += 1;
          if (toggleCounts[2] > 2) {
            toggleCounts[2] = 1;
          }
          switch (toggleCounts[2]) {
            case 1:
              MIDI.sendControlChange(94, 127, trig3MidiChannel);
              clearPixel(5);
              break;
            case 2:
              MIDI.sendControlChange(94, 0, trig3MidiChannel);
              lightNeoPixel(5, 1, color3);
              break;
          }
          noteOn = false;
          delay(500);
          break;
        }
        note3On = true;
        delay(10);
      }
    }
  } else if (sensorValue - tolerance < switch4 && sensorValue + tolerance > switch4) {
    // 4
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note4On) {
        switch (toggleCount) {
        // Trig Mode
        case 1:
          lightNeoPixel(4, 1, color1);
          MIDI.sendNoteOn(pitch, 127, trig4MidiChannel);
          break;
        // Note Mode
        case 2:
          lightNeoPixel(4, 1, color2);
          MIDI.sendNoteOn(pitch4, 127, noteMidiChannel);
          break;
        // Play Mode
        case 3:
          toggleCounts[3] += 1;
          if (toggleCounts[3] > 2) {
            toggleCounts[3] = 1;
          }
          switch (toggleCounts[3]) {
            case 1:
              MIDI.sendControlChange(94, 127, trig4MidiChannel);
              clearPixel(4);
              break;
            case 2:
              MIDI.sendControlChange(94, 0, trig4MidiChannel);
              lightNeoPixel(4, 1, color3);
              break;
          }
          noteOn = false;
          delay(500);
          break;
        }
        note4On = true;
        delay(10);
      }
    }
  } else if (sensorValue - tolerance < switch5 && sensorValue + tolerance > switch5) {
    // 5
    pressCount += 1;
    if (pressCount > pressTol) {
      noteOn = true;
      if (!note5On) {
        switch (toggleCount) {
        // Trig Mode
        case 1:
          lightNeoPixel(3, 1, color1);
          MIDI.sendNoteOn(pitch, 127, trig5MidiChannel);
          break;
        // Note Mode
        case 2:
          lightNeoPixel(3, 1, color2);
          MIDI.sendNoteOn(pitch5, 127, noteMidiChannel);
          break;
        // Play Mode
        case 3:
          toggle5Count += 1;
          toggleCounts[4] += 1;
          if (toggleCounts[4] > 2) {
            toggle5Count = 1;
            toggleCounts[4] = 1;
          }
          switch (toggleCounts[4]) {
            case 1:
              MIDI.sendControlChange(94, 127, trig5MidiChannel);
              clearPixel(3);
              break;
            case 2:
              MIDI.sendControlChange(94, 0, trig5MidiChannel);
              lightNeoPixel(3, 1, color3);
              break;
          }
          noteOn = false;
          delay(500);
          break;
        }
        note5On = true;
        delay(10);
      }
    }
  } else if (sensorValue - tolerance < switch6 && sensorValue + tolerance > switch6) {
    // 6 - toggle
    pressCount += 1;
    if (pressCount > pressTol) {
      toggleCount += 1;
      if (toggleCount > toggleNumber) {
        toggleCount = 1;
      }
      switch (toggleCount) {
        case 1:
          lightNeoPixel(0, 8, color1);
          break;
        case 2:
          lightNeoPixel(0, 8, color2);
          break;
        case 3:
          lightNeoPixel(0, 8, color3);
          break;
      }
      // if (sendMidi) {
      //   lightNeoPixel(0, 8, 200);
      // } else {
      //   lightNeoPixel(0, 8, 100);
      // }
      // sendMidi = !sendMidi;
      pressCount = 0;
      delay(1000);
      clearNeoPixel();
    }
  } else if (sensorValue - tolerance < switch0 && sensorValue + tolerance > switch0) {
    // 0 - release
    if (noteOn) {
      MIDI.sendNoteOff(pitch, 0, trig1MidiChannel);
      MIDI.sendNoteOff(pitch, 0, trig2MidiChannel);
      MIDI.sendNoteOff(pitch, 0, trig3MidiChannel);
      MIDI.sendNoteOff(pitch, 0, trig4MidiChannel);
      MIDI.sendNoteOff(pitch, 0, trig5MidiChannel);
      MIDI.sendNoteOff(pitch1, 0, noteMidiChannel);
      MIDI.sendNoteOff(pitch2, 0, noteMidiChannel);
      MIDI.sendNoteOff(pitch3, 0, noteMidiChannel);
      MIDI.sendNoteOff(pitch4, 0, noteMidiChannel);
      MIDI.sendNoteOff(pitch5, 0, noteMidiChannel);
      noteOn = false;
      clearNeoPixel();
    }
    note1On = false;
    note2On = false;
    note3On = false;
    note4On = false;
    note5On = false;
    switch (toggleCount) {
      case 1:
        lightNeoPixel(0, 1, color1);
        break;
      case 2:
        lightNeoPixel(0, 1, color2);
        break;
      case 3:
        lightNeoPixel(0, 1, color3);
        for (byte i = 0; i < sizeof(toggleCounts) - 1; i++) {
          switch (toggleCounts[i]) {
            case 1:
              clearPixel(pixelNums[i]);
              break;
            case 2:
              lightNeoPixel(pixelNums[i], 1, color3);
              break;
          }
        }
        break;
    }
    pressCount = 0;
  }
  delay(1);        // delay in between reads for stability
}
// ----------------------------------------------------------------------------------
// LED FUNCTIONS
// ----------------------------------------------------------------------------------
void clearNeoPixel() {
  for(int i = 0; i<NUM_PIXELS; i++){
    neoPixel.setPixelColor(i, 0x000000);
    neoPixel.show();
  }
}

void clearPixel(byte i) {
  neoPixel.setPixelColor(i, 0x000000);
  neoPixel.show();
}

void lightNeoPixel(int startIndex, int nPixels, int colorIdx) {
  for(int i=startIndex;i<startIndex+nPixels;i++){
    neoPixel.setPixelColor(i, colorWheel(colorIdx));
    neoPixel.show();
  }
}

uint32_t colorWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return neoPixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return neoPixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return neoPixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}

//  for(int i = 0; i < 5 i++){
//    if (sensorValue - tolerance < switches[i] && sensorValue + tolerance > switches[i]) {
//      pressCount += 1;
//      if (pressCount > pressTol && !noteOn) {
//        switch (toggleCount) {
//        // Trig Mode
//        case 1:
//          lightNeoPixel(7, 1, color1);
//          MIDI.sendNoteOn(pitch, 127, trig1MidiChannel);
//          break;
//        // Note Mode
//        case 2:
//          lightNeoPixel(7, 1, color2);
//          MIDI.sendNoteOn(pitch1, 127, noteMidiChannel);
//          break;
//        // Play Mode
//        case 3:
//          lightNeoPixel(7, 1, color3);
//          MIDI.sendStart();
//          break;
//        }
//        noteOn = true;
//      }
//    }
//    myarray[i]= random(100)
//    neoPixel.setPixelColor(i, 0x000000);
//    neoPixel.show();
//  }
//
