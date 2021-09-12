# Midi Footswitch using TC HELICON SWITCH-6
Audio and MIDI controllable LED animations.

## Reading resistance values from switches
Connect a stereo 1/4" female jack to the arduino as follows:
- Tip : A0
- Middle : 5V
- Sleeve : GND

Upload the following sketch to read the values:

```
void setup() {
  Serial.begin(9600);
}

void loop() {
  int sensorValue = analogRead(A0);
  Serial.println(sensorValue);
  delay(1);        // delay in between reads for stability
}
```

I got the following values:
- 1 : 878
- 2 : 731
- 3 : 585
- 4 : 438
- 5 : 291
- 6 : 145

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
