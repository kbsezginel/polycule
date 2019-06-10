// ----------------------------------------------------------------------------------
// POLYCULE | ONE-SHOT '|' Audio Sample & MP3 Player Arduino Script
// ----------------------------------------------------------------------------------
#include <SoftwareSerial.h>
#include <DFMiniMp3.h>
#include <Keypad.h>
#include <Adafruit_NeoPixel.h>
// ----------------------------------------------------------------------------------
// ------------------------------- KEYPAD SETUP -------------------------------------
// ----------------------------------------------------------------------------------
const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns
char keys[ROWS][COLS] = {
  {1, 2, 3, 4},
  {5, 6, 7, 8},
  {9, 10, 11, 12},
  {13, 14, 15, 16}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
// ----------------------------------------------------------------------------------
// ---------------------------- NEOPIXEL LED SETUP ----------------------------------
// ----------------------------------------------------------------------------------
#define PIN            A5
#define NUMPIXELS      8
long LED_COLOR;
long LED_COLOR1;
long LED_COLOR2;
long LED_COLOR3;
long LED_COLOR4;
int LED_SPEED = 30;
int LED_WIDTH = 8;
int LED_BRIGHTNESS = 70;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
// ----------------------------------------------------------------------------------
// ---------------------- POTENTIOMETER and SWITCH SETUP ----------------------------
// ----------------------------------------------------------------------------------
const int switch1Pin = 12;    // switch input 1
const int switch2Pin = 13;    // switch input 2
const int potPin = A4;
int potValue;
int potVolume;
int potLED;
int potMAP;
// ----------------------------------------------------------------------------------
// ------------------------------ MP3 PLAYER SETUP ----------------------------------
// ----------------------------------------------------------------------------------
int WAIT_TIME = 200;
int SAMPLE_SET = 1;
const int MAX_VOLUME = 30;
int TRACK_NO = 1;
int NUM_MP3_TRACKS = 16;
class Mp3Notify
{
public:
  static void OnError(uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }

  static void OnPlayFinished(uint16_t globalTrack)
  {
    Serial.println();
    Serial.print("Play finished for #");
    Serial.println(globalTrack);
  }

  static void OnCardOnline(uint16_t code)
  {
    Serial.println();
    Serial.print("Card online ");
    Serial.println(code);
  }

  static void OnCardInserted(uint16_t code)
  {
    Serial.println();
    Serial.print("Card inserted ");
    Serial.println(code);
  }

  static void OnCardRemoved(uint16_t code)
  {
    Serial.println();
    Serial.print("Card removed ");
    Serial.println(code);
  }
};
SoftwareSerial secondarySerial(10, 11); // RX, TX
DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(secondarySerial);
// ----------------------------------------------------------------------------------
// --------------------------------- END SETUP --------------------------------------
// ----------------------------------------------------------------------------------
// ##################################################################################
void setup()
{
  Serial.begin(115200);
  Serial.begin(9600);  // This initializes keypad

  mp3.begin();
  uint16_t volume = mp3.getVolume();
  mp3.setVolume(24);

  pixels.begin();      // This initializes the NeoPixel library.
  clearPixels();       // Clear all LEDs

  LED_COLOR1 = colorWheel(28);   // Yellow
  LED_COLOR2 = colorWheel(20);   // Red
  LED_COLOR3 = colorWheel(0);    // Purple
  LED_COLOR4 = colorWheel(150);  // Blue
  LED_COLOR = LED_COLOR1;

  ledAnim(1, 32, 8, LED_COLOR1, false);
  ledAnim(1, 32, 8, LED_COLOR2, false);
  ledAnim(1, 32, 8, LED_COLOR1, true);

  pinMode(potPin, INPUT);
  // set the switch as an input:
  pinMode(switch1Pin, INPUT);
  pinMode(switch2Pin, INPUT);
}
// ##################################################################################
void loop()
{

   char key = keypad.getKey();
   // AUDIO SAMPLE MODE -------------------------------------------------------------
   if (digitalRead(switch1Pin) == HIGH) {
      // ledAnim(1, LED_SPEED, LED_WIDTH, LED_COLOR, true);
      // PLAY SAMPLE
      if (key){
        mp3.playFolderTrack(SAMPLE_SET, key);
        mp3.loop();
        lightPixels(key, LED_COLOR, LED_BRIGHTNESS);
        waitMilliseconds(WAIT_TIME);
      }
     // SELECT SAMPLE SET
     switch(key){
      case 13: SAMPLE_SET = 1; LED_COLOR = LED_COLOR1; ledAnim(1, LED_SPEED, LED_WIDTH, LED_COLOR, true); break;
      case 14: SAMPLE_SET = 2; LED_COLOR = LED_COLOR2; ledAnim(1, LED_SPEED, LED_WIDTH, LED_COLOR, true); break;
      case 15: SAMPLE_SET = 3; LED_COLOR = LED_COLOR3; ledAnim(1, LED_SPEED, LED_WIDTH, LED_COLOR, true); break;
      case 16: SAMPLE_SET = 4; LED_COLOR = LED_COLOR4; ledAnim(1, LED_SPEED, LED_WIDTH, LED_COLOR, true); break;
     }
   }
   // MP3 PLAYER AND SETTINGS MODE ---------------------------------------------------
   else if(digitalRead(switch2Pin) == HIGH){
    switch(key){
      case 1: mp3.start(); lightPixels(TRACK_NO, LED_COLOR, LED_BRIGHTNESS); break;
      case 2: mp3.pause(); waitMilliseconds(WAIT_TIME); break;
      case 3: TRACK_NO += 1; mp3.playMp3FolderTrack(TRACK_NO); lightPixels(TRACK_NO, LED_COLOR, LED_BRIGHTNESS); break;
      case 4: TRACK_NO -= 1; mp3.playMp3FolderTrack(TRACK_NO); lightPixels(TRACK_NO, LED_COLOR, LED_BRIGHTNESS); break;
      case 5: TRACK_NO = random(1, NUM_MP3_TRACKS); mp3.playMp3FolderTrack(TRACK_NO); lightPixels(TRACK_NO, LED_COLOR, LED_BRIGHTNESS); break;
      case 6: TRACK_NO = 1; mp3.playMp3FolderTrack(TRACK_NO); lightPixels(TRACK_NO, LED_COLOR, LED_BRIGHTNESS); break;
      case 9: ledAnim(1, 50, 6, LED_COLOR1, false); ledAnim(1, 50, 6, LED_COLOR1, false); ledAnim(2, 30, 8, LED_COLOR2, true); break;
      case 10: ledAnim(1, 50, 4, LED_COLOR3, false); ledAnim(1, 50, 4, LED_COLOR3, false); ledAnim(2, 30, 8, LED_COLOR2, true); break;
      case 11: ledAnim(1, 70, 8, LED_COLOR4, false); ledAnim(1, 70, 8, LED_COLOR4, false); ledAnim(2, 30, 8, LED_COLOR3, true); break;
      case 12: ledAnim(1, 100, 8, LED_COLOR2, false); ledAnim(1, 100, 8, LED_COLOR1, false); ledAnim(2, 30, 8, LED_COLOR2, true); break;
      case 13:
        potValue = analogRead(potPin);
        potLED = map(potValue, 0, 1023, 0, 8);
        potMAP = map(potValue, 0, 1023, 0, 30);
        mp3.setVolume(potMAP);
        lightPixels(potLED, LED_COLOR, LED_BRIGHTNESS);
        break;
      case 14:
        potValue = analogRead(potPin);
        potLED = map(potValue, 0, 1023, 0, 8);
        potMAP = map(potValue, 0, 1023, 0, 127);
        LED_BRIGHTNESS = potMAP;
        lightPixels(8, LED_COLOR, LED_BRIGHTNESS);
        break;
      case 15:
        potValue = analogRead(potPin);
        potLED = map(potValue, 0, 1023, 0, 8);
        potMAP = map(potValue, 0, 1023, 50, 1000);
        WAIT_TIME = potMAP;
        lightPixels(potLED, LED_COLOR, LED_BRIGHTNESS);
        break;
      case 16:
        potValue = analogRead(potPin);
        potMAP = map(potValue, 0, 1023, 0, 250);
        LED_COLOR = colorWheel(potMAP);
        lightPixels(8, LED_COLOR, LED_BRIGHTNESS);
     }
   }
   mp3.loop();
}
// ##################################################################################
// Potentiometer
void adjustParameter(int key, int potPin){
  int potValue = analogRead(potPin);
  potLED = map(potValue, 0, 1023, 0, 8);
  int potMAP;
  switch(key){
    case 13:
      potMAP = map(potValue, 0, 1023, 0, 30);
      mp3.setVolume(potMAP);
      break;
    case 14:
      potMAP = map(potValue, 0, 1023, 0, 127);
      LED_BRIGHTNESS = potMAP;
      lightPixels(8, LED_COLOR, LED_BRIGHTNESS);
      break;
    case 15: mp3.prevTrack();
      potMAP = map(potValue, 0, 1023, 50, 1000);
      WAIT_TIME = potMAP;
      break;
    case 16: mp3.playRandomTrackFromAll(); break;
   }
}
int readPot(int potPin, int minValue, int maxValue)
{
  potValue = analogRead(potPin);
  int potMAP = map(potValue, 0, 1023, minValue, maxValue);
  return potMAP;
}
// ----------------------------------------------------------------------------------
// LED Color Wheel
// ----------------------------------------------------------------------------------
void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  while ((millis() - start) < msWait)
  {
    mp3.loop();
    delay(1);
  }
}
// ----------------------------------------------------------------------------------
// LED light given number of pixels
// ----------------------------------------------------------------------------------
void lightPixels(uint16_t nPixels, uint32_t color, uint16_t brightness){
    clearPixels();
    for(int i=0; i < nPixels; i++){
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, color);
      pixels.setBrightness(brightness);
      pixels.show();
      // delay(delayval);
  }
}
// ----------------------------------------------------------------------------------
// LED Clear Pixels
// ----------------------------------------------------------------------------------
void clearPixels() {
  for( int i = 0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i, 0x000000);
    pixels.show();
  }
}
// ----------------------------------------------------------------------------------
// LED Color Wheel
// ----------------------------------------------------------------------------------
uint32_t colorWheel(byte WheelPos) {
  byte state = WheelPos / 21;
  switch(state) {
    case 0: return pixels.Color(255, 0, 255 - ((((WheelPos % 21) + 1) * 6) + 127)); break;
    case 1: return pixels.Color(255, ((WheelPos % 21) + 1) * 6, 0); break;
    case 2: return pixels.Color(255, (((WheelPos % 21) + 1) * 6) + 127, 0); break;
    case 3: return pixels.Color(255 - (((WheelPos % 21) + 1) * 6), 255, 0); break;
    case 4: return pixels.Color(255 - (((WheelPos % 21) + 1) * 6) + 127, 255, 0); break;
    case 5: return pixels.Color(0, 255, ((WheelPos % 21) + 1) * 6); break;
    case 6: return pixels.Color(0, 255, (((WheelPos % 21) + 1) * 6) + 127); break;
    case 7: return pixels.Color(0, 255 - (((WheelPos % 21) + 1) * 6), 255); break;
    case 8: return pixels.Color(0, 255 - ((((WheelPos % 21) + 1) * 6) + 127), 255); break;
    case 9: return pixels.Color(((WheelPos % 21) + 1) * 6, 0, 255); break;
    case 10: return pixels.Color((((WheelPos % 21) + 1) * 6) + 127, 0, 255); break;
    case 11: return pixels.Color(255, 0, 255 - (((WheelPos % 21) + 1) * 6)); break;
    default: return pixels.Color(0, 0, 0); break;
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
  uint32_t old_val[NUMPIXELS]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 1; count<NUMPIXELS; count++) {
      pixels.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        pixels.setPixelColor(x-1, old_val[x-1]);
      }
      pixels.show();
      delay(speed);
    }
    for (int count = NUMPIXELS-1; count>=0; count--) {
      pixels.setPixelColor(count, color);
      old_val[count] = color;
      for(int x = count; x<=NUMPIXELS ;x++) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        pixels.setPixelColor(x+1, old_val[x+1]);
      }
      pixels.show();
      delay(speed);
    }
  }
  if (clearAll){
    clearPixels();
  }
}
