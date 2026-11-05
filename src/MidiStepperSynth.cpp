#include "MidiStepperSynth.h"

#include "backends/ToneBackend.h"

static float noteToFreq(float note) {
  return 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);
}

MidiStepperSynth::MidiStepperSynth(const uint8_t *stepPins, uint8_t numMotors,
                                   const uint8_t *dirPins)
    : stepPins_(stepPins), dirPins_(dirPins),
      numVoices_(numMotors < MSS_MAX_VOICES ? numMotors : MSS_MAX_VOICES) {}

uint8_t MidiStepperSynth::maxMotors() { return mss::toneMaxVoices(); }

bool MidiStepperSynth::begin() {
  if (dirPins_) {
    for (uint8_t v = 0; v < numVoices_; v++) {
      pinMode(dirPins_[v], OUTPUT);
      digitalWrite(dirPins_[v], LOW); // direction doesn't matter for pitch
    }
  }
  if (!mss::toneBegin(stepPins_, numVoices_)) return false;
  allNotesOff();
  return true;
}

// ---------------- Voices ----------------
void MidiStepperSynth::sound(uint8_t v) {
  float freq = noteToFreq(voices_[v].note + voices_[v].bend);
  if (freq < 16.0f) freq = 16.0f;
  mss::toneSet(v, freq);
}

void MidiStepperSynth::silence(uint8_t v) {
  voices_[v].note = -1;
  mss::toneStop(v);
}

void MidiStepperSynth::noteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  uint8_t v = channel - 1;
  if (v >= numVoices_) return;
  if (velocity == 0) { // note on with velocity 0 = note off
    noteOff(channel, note);
    return;
  }
  voices_[v].note = note; // last-note priority per channel
  sound(v);
}

void MidiStepperSynth::noteOff(uint8_t channel, uint8_t note) {
  uint8_t v = channel - 1;
  if (v >= numVoices_) return;
  if (voices_[v].note == (int8_t)note) silence(v);
}

void MidiStepperSynth::pitchBend(uint8_t channel, int bend) {
  uint8_t v = channel - 1;
  if (v >= numVoices_) return;
  voices_[v].bend = (bend / 8192.0f) * bendRange_;
  if (voices_[v].note >= 0) sound(v);
}

void MidiStepperSynth::allNotesOff() {
  for (uint8_t v = 0; v < numVoices_; v++) silence(v);
}

// ---------------- MIDI byte-stream parser ----------------
// Handles running status. Ignores system messages and unsupported types.
void MidiStepperSynth::read(Stream &in) {
  while (in.available() > 0) handleMidiByte((uint8_t)in.read());
}

void MidiStepperSynth::handleMidiByte(uint8_t b) {
  if (b >= 0xF8) return; // real-time messages: ignore entirely
  if (b >= 0xF0) {       // system common: cancels running status
    status_ = 0;
    dataCount_ = 0;
    return;
  }
  if (b & 0x80) { // new status byte
    status_ = b;
    dataCount_ = 0;
    uint8_t type = b & 0xF0;
    dataNeeded_ = (type == 0xC0 || type == 0xD0) ? 1 : 2;
    return;
  }
  if (status_ == 0) return; // stray data byte before any status
  data_[dataCount_++] = b;
  if (dataCount_ >= dataNeeded_) {
    dispatch();
    dataCount_ = 0; // running status: keep status, wait for new data
  }
}

void MidiStepperSynth::dispatch() {
  uint8_t channel = (status_ & 0x0F) + 1;
  switch (status_ & 0xF0) {
    case 0x90: noteOn(channel, data_[0], data_[1]); break;
    case 0x80: noteOff(channel, data_[0]); break;
    case 0xE0: pitchBend(channel, ((data_[1] << 7) | data_[0]) - 8192); break;
    case 0xB0:
      if (data_[0] == 120 || data_[0] == 123) { // all sound / notes off
        uint8_t v = channel - 1;
        if (v < numVoices_) silence(v);
      }
      break;
    default: break; // other message types ignored
  }
}
