# Midi Footswitch using TC HELICON SWITCH-6
Turning a foot pedal to a midi controller using Arduino.

First we need to figure out how we can read the individual presses from the pedal. The pedal output is a female stereo jack and when a voltage is applied between the middle and sleeve connections different voltage values can be read from the tip for different switches. This is because every switch is connected to a resistor in parallel and the total resistance changes according to which switch you are pressing. [See here for the wiring diagram](https://www.just-jamie.com/2015/02/07/tc-helicon-switch-6-schematic/).

## Reading resistance values from switches
Connect one end of a stereo 1/4" jack to the pedal and the other end to the arduino using banana cables as follows:
- Tip : A0
- Middle : 5V
- Sleeve : GND

Upload the following sketch to read the values:

```
void setup() {
  // Add a pull-up resistor to avoid floating voltages when no switch is pressed
  pinMode(A0, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  int sensorValue = analogRead(A0);
  Serial.println(sensorValue);
  delay(1);        // delay in between reads for stability
}
```

I got the following values:
- 1 : 905
- 2 : 813
- 3 : 724
- 4 : 626
- 5 : 500
- 6 : 321

Now we can use these values to differentiate between different switches. The read values are not perfect therefore we are going to add a tolerance value to detect presses. So instead of checking if the read is exactly 905 we will check if it's between 905 - tolerance and 905 + tolerance. We will add another check to make it a bit more robust. Instead of confirming the press as soon as we get a single read in the corerct range, we are going to check for consistent reads. We can do this by counting the number of reads in the same range and confirming the press after a certain number of reads. Eventually this will look something like:

```
byte tolerance = 3;
byte pressTol = 50;

void loop() {
  int sensorValue = analogRead(A0);
  if (sensorValue - tolerance < 905 && sensorValue + tolerance > 905) {
    pressCount += 1;
    if (pressCount > pressTol) {
      // press confirmed! do stuff
    }
  }
}
```

### MIDI output circuit
https://www.midi.org/midi-articles/arduino-midi-output-basics

https://www.notesandvolts.com/2015/03/midi-for-arduino-build-midi-output.html

- Pin 4 : 220 ohm resistor -> 5V
- Pin 2 : GND
- Pin 5 : TX



### Neopixel


# V2 (Midi over USB)
https://www.pjrc.com/teensy/td_midi.html

## Code

```

```

### Resources
- Digitakt MIDI CCs : https://midi.user.camp/d/elektron/digitakt/
