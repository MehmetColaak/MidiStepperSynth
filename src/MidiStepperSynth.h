// MidiStepperSynth — play MIDI on stepper motors, one motor per MIDI channel.
//
// Works with any STEP/DIR driver (A4988, DRV8825, TMC2208 ...). The pitch of
// each motor is the frequency of its STEP pulses.

#pragma once

#include <Arduino.h>

#ifndef MSS_MAX_VOICES
#define MSS_MAX_VOICES 16
#endif

class MidiStepperSynth {
 public:
  // stepPins: one STEP pin per motor; motor 1 plays MIDI channel 1, etc.
  // dirPins:  optional DIR pins (same order), or nullptr if not wired.
  MidiStepperSynth(const uint8_t *stepPins, uint8_t numMotors,
                   const uint8_t *dirPins = nullptr);

  // Sets up the pins and tone hardware. Returns false if this board can't
  // drive that many motors.
  bool begin();

  // Raw MIDI input: call read(Serial) from loop(), or feed bytes yourself.
  void read(Stream &in);
  void handleMidiByte(uint8_t b);

  // Direct control. Channels are 1..16, like the Arduino MIDI library.
  void noteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  void noteOff(uint8_t channel, uint8_t note);
  void pitchBend(uint8_t channel, int bend); // -8192..8191
  void allNotesOff();

  void setBendRange(float semitones) { bendRange_ = semitones; }

  // How many motors the tone hardware on this board can drive.
  static uint8_t maxMotors();

 private:
  struct Voice {
    int8_t note = -1;  // sounding MIDI note, -1 = silent
    float bend = 0.0f; // semitones
  };

  void silence(uint8_t v);
  void sound(uint8_t v);
  void dispatch();

  const uint8_t *stepPins_;
  const uint8_t *dirPins_;
  uint8_t numVoices_;
  float bendRange_ = 2.0f;
  Voice voices_[MSS_MAX_VOICES];

  // MIDI byte-stream parser state
  uint8_t status_ = 0;
  uint8_t data_[2];
  uint8_t dataCount_ = 0;
  uint8_t dataNeeded_ = 0;
};
