// ----------------------------------------------------------------------------------
// POLYCULE | LUNT | Light Bulb Animation
// ----------------------------------------------------------------------------------
#include "dimmable_light.h"

#define DIM_SYNC_PIN 2
#define DIM1_PIN 11
#define DIM2_PIN 3
#define DIM3_PIN 3
#define DIM4_PIN 3

DimmableLight dimLight1(DIM1_PIN);
DimmableLight dimLight2(DIM2_PIN);
DimmableLight dimLight3(DIM3_PIN);
DimmableLight dimLight4(DIM4_PIN);

// Delay between a brightness changement in millisecond
int period = 50;
int dly = 500;
int minBrightness = 80;
int maxBrightness = 250;

void setup() {
  DimmableLight::setSyncPin(DIM_SYNC_PIN);
  DimmableLight::begin();
}

void loop() {
  for(int i=minBrightness;i<maxBrightness;i++){
    dimLight1.setBrightness(i);
    delay(period);
  }
  for(int i=maxBrightness;i>minBrightness;i--){
    dimLight1.setBrightness(i);
    delay(period);
  }
}
