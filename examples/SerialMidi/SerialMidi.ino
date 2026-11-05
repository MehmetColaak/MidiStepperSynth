// SerialMidi — play raw MIDI bytes arriving over USB serial.
//
// Motor 1 plays MIDI channel 1, motor 2 channel 2, and so on.
// Send MIDI from Max/MSP's [serial] object, or any tool that writes raw MIDI
// bytes to the serial port.
//
// Wiring per motor: driver STEP -> step pin, DIR -> dir pin, common GND.
// Pins below are for an ESP32-S3 with 5 motors — change them for your board.

#include <MidiStepperSynth.h>

const uint8_t STEP_PINS[] = {4, 5, 6, 7, 15};
const uint8_t DIR_PINS[] = {16, 17, 18, 8, 3};

MidiStepperSynth synth(STEP_PINS, sizeof(STEP_PINS), DIR_PINS);

void setup() {
  Serial.begin(115200);
  if (!synth.begin()) {
    Serial.print("This board can drive at most ");
    Serial.print(MidiStepperSynth::maxMotors());
    Serial.println(" motors");
  }
}

void loop() {
  synth.read(Serial);
}
