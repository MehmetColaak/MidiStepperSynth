#include "MidiStepperSynth.h"

#include "backends/ToneBackend.h"

static float noteToFreq(float note) {
  return 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);
}

MidiStepperSynth::MidiStepperSynth(const uint8_t *stepPins, uint8_t numMotors,
                                   const uint8_t *dirPins)
    : stepPins_(stepPins), dirPins_(dirPins),
      numVoices_(numMotors < MSS_MAX_VOICES ? numMotors : MSS_MAX_VOICES) {
  for (uint8_t v = 0; v < numVoices_; v++) order_[v] = v;
}

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

void MidiStepperSynth::setPolyphonic(bool on) {
  allNotesOff();
  poly_ = on;
}

void MidiStepperSynth::setNoteRange(uint8_t low, uint8_t high) {
  if (high > 127 || high < low + 11) return; // needs a full octave to fold into
  low_ = low;
  high_ = high;
}

// ---------------- Voices ----------------
void MidiStepperSynth::sound(uint8_t v) {
  const Voice &vc = voices_[v];
  float freq = noteToFreq(vc.pitch + bend_[vc.channel - 1]);
  if (freq < 16.0f) freq = 16.0f;
  mss::toneSet(v, freq);
}

void MidiStepperSynth::silence(uint8_t v) {
  voices_[v].note = -1;
  mss::toneStop(v);
}

// Mark voice v as the most recently used.
void MidiStepperSynth::touch(uint8_t v) {
  uint8_t i = 0;
  while (order_[i] != v) i++;
  for (; i + 1 < numVoices_; i++) order_[i] = order_[i + 1];
  order_[numVoices_ - 1] = v;
}

// Free voice idle the longest, else the one playing the oldest note.
uint8_t MidiStepperSynth::pickVoice() {
  for (uint8_t i = 0; i < numVoices_; i++) {
    if (voices_[order_[i]].note < 0) return order_[i];
  }
  return order_[0];
}

void MidiStepperSynth::start(uint8_t v, uint8_t channel, uint8_t note) {
  uint8_t pitch = note;
  while (pitch < low_) pitch += 12;
  while (pitch > high_) pitch -= 12;
  voices_[v].note = note;
  voices_[v].pitch = pitch;
  voices_[v].channel = channel;
  touch(v);
  sound(v);
}

void MidiStepperSynth::noteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (channel < 1 || channel > 16) return;
  if (velocity == 0) { // note on with velocity 0 = note off
    noteOff(channel, note);
    return;
  }
  if (poly_) {
    start(pickVoice(), channel, note);
  } else if (channel <= numVoices_) {
    start(channel - 1, channel, note); // last-note priority per channel
  }
}

void MidiStepperSynth::noteOff(uint8_t channel, uint8_t note) {
  for (uint8_t v = 0; v < numVoices_; v++) {
    if (voices_[v].note == (int8_t)note && voices_[v].channel == channel) {
      silence(v);
    }
  }
}

void MidiStepperSynth::pitchBend(uint8_t channel, int bend) {
  if (channel < 1 || channel > 16) return;
  bend_[channel - 1] = (bend / 8192.0f) * bendRange_;
  for (uint8_t v = 0; v < numVoices_; v++) {
    if (voices_[v].note >= 0 && voices_[v].channel == channel) sound(v);
  }
}

void MidiStepperSynth::allNotesOff() {
  for (uint8_t v = 0; v < numVoices_; v++) silence(v);
}

// ---------------- MIDI input ----------------
void MidiStepperSynth::read(Stream &in) {
  while (in.available() > 0) handleMidiByte((uint8_t)in.read());
}

#if MSS_HAS_USB_MIDI
void MidiStepperSynth::readUsbMidi() {
  uint8_t p[4];
  while (mss::usbMidiRead(p)) {
    uint8_t cin = p[0] & 0x0F; // code index number = message type
    if (cin < 0x8 || cin > 0xE) continue; // only channel messages
    uint8_t len = (cin == 0xC || cin == 0xD) ? 2 : 3;
    for (uint8_t i = 1; i <= len; i++) handleMidiByte(p[i]);
  }
}
#endif

// Byte-stream parser. Handles running status; ignores system messages and
// unsupported types.
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
        for (uint8_t v = 0; v < numVoices_; v++) {
          if (voices_[v].channel == channel) silence(v);
        }
      }
      break;
    default: break; // other message types ignored
  }
}
