# MidiStepperSynth

Play MIDI on stepper motors. Each motor plays one MIDI channel; the pitch is
the frequency of its STEP pulses. Works with any STEP/DIR driver
(A4988, DRV8825, TMC2208, ...).

```cpp
#include <MidiStepperSynth.h>

const uint8_t STEP_PINS[] = {4, 5, 6, 7, 15};
const uint8_t DIR_PINS[]  = {16, 17, 18, 8, 3};   // optional

MidiStepperSynth synth(STEP_PINS, sizeof(STEP_PINS), DIR_PINS);

void setup() {
  Serial.begin(115200);
  synth.begin();
}

void loop() {
  synth.read(Serial);   // raw MIDI bytes over serial
}
```

## Boards

| Board | Motors | Tone generation |
|---|---|---|
| ESP32 | 14 | LEDC + MCPWM hardware PWM |
| ESP32-S3 | 10 | LEDC + MCPWM hardware PWM |
| ESP32-S2 | 4 | LEDC hardware PWM |
| ESP32-C3 / C6 / H2 | 3 | LEDC hardware PWM |
| AVR, SAMD, Teensy, others | planned | timer interrupt |

Arduino-ESP32 core 2.x and 3.x are both supported.

## Wiring

Per motor: driver STEP → step pin, DIR → dir pin (optional), and a common GND
between the board and the drivers. Power the driver logic (VDD) from the
board's 3.3 V or 5 V to match its I/O level, and the motor (VMOT) from a
separate supply.

## MIDI

- Note On / Note Off, last-note priority per channel
- Pitch Bend (±2 semitones, `setBendRange()`)
- CC 120 / 123 (all sound / notes off)

Feed it raw MIDI with `read(Stream&)` / `handleMidiByte()`, or call
`noteOn()`, `noteOff()`, `pitchBend()` yourself, e.g. from the
[Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library)
callbacks.

## License

MIT
