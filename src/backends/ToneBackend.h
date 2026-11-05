// Tone backend: generates a square wave of a given frequency on each STEP pin.
// One implementation per board family, picked at compile time.

#pragma once

#include <Arduino.h>

#if !defined(ARDUINO_ARCH_ESP32)
#error "MidiStepperSynth: only ESP32 boards are supported so far."
#endif

namespace mss {

uint8_t toneMaxVoices();
bool toneBegin(const uint8_t *pins, uint8_t count);
void toneSet(uint8_t voice, float hz);
void toneStop(uint8_t voice);

} // namespace mss
