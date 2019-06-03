# Moduled Nano
MIDI controlled modular LED synthesizer (8 hp).
Setup LED animations using MIDI data on your modular rack.

<p align="center"><img src="assets/img/moduled/moduled-nano-front.png" width="100"></p>

## DIY

## Hardware

### What you need
- 1x Arduino Nano
- 1x 8x8 LED matrix
- 1x Neopixel LED ring (8 LEDs)
- 1x MIDI female jack
- 2x buttons
- 1x 2-way toggle switch
- 2x 10K Potentiometers
- 2x 220 ohm resistor
- 1x 4.7k resistor
- 3x 10k resistor
- 1x 1N914 diode
- 1x 6N138 Optocoupler
- Protoboard, wires, soldering iron

### Case
I designed the case using `Blender` 3D rendering software and exported an `stl` file and printed the case using Lulzbot TAZ 6 3D printer. The `.blend` and `stl` file for the case can be found [here](https://github.com/kbsezginel/polycule/tree/master/moduled).

## Software

### moduled_nano.ino
[Moduled Nano Arduino script.](https://github.com/kbsezginel/polycule/blob/master/moduled/moduled_nano.ino)

### Setting up custom LED animations
Using this [online LED matrix editor](https://xantorohara.github.io/led-matrix-editor/) you can setup a sequence of images to be added as an animation.

## Usage
<p align="center"><img src="assets/img/moduled/moduled-nano-usage.png" width="400"></p>
