// UsbMidi — the board shows up on the computer as a MIDI device, so a DAW,
// keyboard software or Max can play the motors directly.
// Raw MIDI over USB serial keeps working too.
//
// Needs a chip with native USB (ESP32-S2 / ESP32-S3).
// Arduino IDE: Tools > USB Mode > "USB-OTG (TinyUSB)".
// PlatformIO:  build_flags = -D ARDUINO_USB_MODE=0 -D ARDUINO_USB_CDC_ON_BOOT=1
//
// Pins below are for an ESP32-S3 with 5 motors — change them for your board.

#include <MidiStepperSynth.h>

#if !MSS_HAS_USB_MIDI
#error "USB-MIDI needs an ESP32-S2/S3 with USB Mode set to USB-OTG (TinyUSB)"
#endif

const uint8_t STEP_PINS[] = {4, 5, 6, 7, 15};
const uint8_t DIR_PINS[] = {16, 17, 18, 8, 3};

MidiStepperSynth synth(STEP_PINS, sizeof(STEP_PINS), DIR_PINS);

void setup() {
  Serial.begin(115200);
  synth.begin();

  // Optional:
  // synth.setPolyphonic(true);    // play chords from one channel on all motors
  // synth.setNoteRange(36, 84);   // fold notes into C2..C6
}

void loop() {
  synth.readUsbMidi();
  synth.read(Serial);
}
