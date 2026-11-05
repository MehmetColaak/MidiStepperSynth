// USB-MIDI input: the board shows up on the computer as a MIDI device.
// Available on chips with native USB running the TinyUSB stack
// (ESP32-S2, ESP32-S3 with "USB Mode: USB-OTG (TinyUSB)").

#pragma once

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32) && CONFIG_TINYUSB_MIDI_ENABLED && !ARDUINO_USB_MODE
#define MSS_HAS_USB_MIDI 1
#else
#define MSS_HAS_USB_MIDI 0
#endif

#if MSS_HAS_USB_MIDI
namespace mss {

// Reads one USB-MIDI event packet: [cable/code index, status, data1, data2].
bool usbMidiRead(uint8_t packet[4]);

} // namespace mss
#endif
