// MidiStepperSynth — play MIDI on stepper motors.
//
// Works with any STEP/DIR driver (A4988, DRV8825, TMC2208 ...). The pitch of
// each motor is the frequency of its STEP pulses.
//
// Two ways to map notes to motors:
//   - channel per motor (default): motor 1 plays MIDI channel 1, etc.
//   - polyphonic: notes from any channel go to whichever motor is free.

#pragma once

#include <Arduino.h>

#include "backends/UsbMidi.h"

#ifndef MSS_MAX_VOICES
#define MSS_MAX_VOICES 16
#endif

class MidiStepperSynth {
 public:
  // stepPins: one STEP pin per motor.
  // dirPins:  optional DIR pins (same order), or nullptr if not wired.
  MidiStepperSynth(const uint8_t *stepPins, uint8_t numMotors,
                   const uint8_t *dirPins = nullptr);

  // Sets up the pins and tone hardware. Returns false if this board can't
  // drive that many motors.
  bool begin();

  // Raw MIDI input: call read(Serial) from loop(), or feed bytes yourself.
  void read(Stream &in);
  void handleMidiByte(uint8_t b);

#if MSS_HAS_USB_MIDI
  // USB-MIDI input: call from loop().
  void readUsbMidi();
#endif

  // Direct control. Channels are 1..16, like the Arduino MIDI library.
  void noteOn(uint8_t channel, uint8_t note, uint8_t velocity);
  void noteOff(uint8_t channel, uint8_t note);
  void pitchBend(uint8_t channel, int bend); // -8192..8191
  void allNotesOff();

  // Polyphonic mode: each new note goes to the motor idle the longest; when
  // all are busy, the oldest note is cut off.
  void setPolyphonic(bool on);
  // Notes outside low..high are moved by octaves into the range, so motors
  // never get pitches they can't play. Range must span at least an octave.
  void setNoteRange(uint8_t low, uint8_t high);
  void setBendRange(float semitones) { bendRange_ = semitones; }

  // How many motors the tone hardware on this board can drive.
  static uint8_t maxMotors();

 private:
  struct Voice {
    int8_t note = -1;    // MIDI note as received, -1 = silent
    uint8_t pitch = 0;   // note actually played, after octave folding
    uint8_t channel = 0; // 1..16
  };

  void start(uint8_t v, uint8_t channel, uint8_t note);
  void silence(uint8_t v);
  void sound(uint8_t v);
  void touch(uint8_t v);
  uint8_t pickVoice();
  void dispatch();

  const uint8_t *stepPins_;
  const uint8_t *dirPins_;
  uint8_t numVoices_;
  Voice voices_[MSS_MAX_VOICES];
  uint8_t order_[MSS_MAX_VOICES]; // voice indices, least recently used first
  float bend_[16] = {};           // semitones, per MIDI channel
  float bendRange_ = 2.0f;
  bool poly_ = false;
  uint8_t low_ = 0, high_ = 127;

  // MIDI byte-stream parser state
  uint8_t status_ = 0;
  uint8_t data_[2];
  uint8_t dataCount_ = 0;
  uint8_t dataNeeded_ = 0;
};
