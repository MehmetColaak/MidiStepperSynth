// USB-MIDI device on ESP32-S2/S3 via the core's TinyUSB stack.
// The interface is registered by a static constructor, before the core starts
// USB, so the board enumerates as a MIDI device (plus serial, if enabled).

#include "UsbMidi.h"

#if MSS_HAS_USB_MIDI

#include "esp32-hal-tinyusb.h"

static uint16_t loadDescriptor(uint8_t *dst, uint8_t *itf) {
  uint8_t str = tinyusb_add_string_descriptor("MidiStepperSynth");
  uint8_t epIn = tinyusb_get_free_in_endpoint();
  uint8_t epOut = tinyusb_get_free_out_endpoint();
  TU_VERIFY(epIn && epOut);
  uint8_t desc[TUD_MIDI_DESC_LEN] = {
      TUD_MIDI_DESCRIPTOR(*itf, str, epOut, (uint8_t)(0x80 | epIn), 64)};
  *itf += 2; // MIDI uses two interfaces (audio control + MIDI streaming)
  memcpy(dst, desc, TUD_MIDI_DESC_LEN);
  return TUD_MIDI_DESC_LEN;
}

namespace {
struct Register {
  Register() {
    tinyusb_enable_interface(USB_INTERFACE_MIDI, TUD_MIDI_DESC_LEN, loadDescriptor);
  }
} registerUsbMidi;
} // namespace

bool mss::usbMidiRead(uint8_t packet[4]) {
  return tud_midi_available() && tud_midi_packet_read(packet);
}

#endif // MSS_HAS_USB_MIDI
