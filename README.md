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
  // synth.readUsbMidi(); // ESP32-S2/S3: USB-MIDI device
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

## MIDI input

| Input | Boards | How |
|---|---|---|
| Serial (raw MIDI bytes) | all | `synth.read(Serial)` in `loop()` |
| USB-MIDI device | ESP32-S2, ESP32-S3 | `synth.readUsbMidi()` in `loop()`; set *USB Mode: USB-OTG (TinyUSB)* |

With USB-MIDI the board appears on the computer as a MIDI device, so a DAW
or Max can play it directly. See `examples/UsbMidi`.

You can also call `noteOn()`, `noteOff()`, `pitchBend()` yourself, e.g. from
[Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library)
callbacks. Channels are 1–16.

Supported: Note On / Off, Pitch Bend (±2 semitones, `setBendRange()`),
CC 120 / 123 (all sound / notes off).

## Note mapping

- **Channel per motor** (default): motor 1 plays channel 1, motor 2 channel 2…
  one note at a time, the newest wins.
- **Polyphonic**: `synth.setPolyphonic(true)` — notes from any channel go to
  the motor idle the longest. When all motors are busy, the oldest note is cut
  off. Play chords from a single keyboard or a single-track MIDI file.
- **Note range**: `synth.setNoteRange(36, 84)` — notes outside the range are
  moved by octaves into it, so motors never get pitches they can't play.

## License

MIT
