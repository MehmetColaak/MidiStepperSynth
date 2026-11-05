// ESP32 tone backend — hardware PWM, no CPU load while notes play.
//
// First voices use LEDC (one LEDC timer each, so every voice gets its own
// frequency), the rest use MCPWM timers where the chip has them.
//
//   chip            LEDC  MCPWM  total
//   ESP32             8     6     14
//   ESP32-S3          4     6     10
//   ESP32-S2          4     0      4
//   C3 / C6 / H2      3     0      3
//
// Supports Arduino-ESP32 core 2.x and 3.x.

#if defined(ARDUINO_ARCH_ESP32)

#include "ToneBackend.h"

#if CONFIG_IDF_TARGET_ESP32
#define MSS_LEDC_VOICES 8
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define MSS_LEDC_VOICES 4
#else
#define MSS_LEDC_VOICES 3
#endif

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3
#define MSS_MCPWM_VOICES 6
#include "driver/mcpwm.h"
#else
#define MSS_MCPWM_VOICES 0
#endif

namespace mss {

static const uint8_t *pins_;

// Even LEDC channels sit on separate timers (channel / 2 = timer).
static uint8_t ledcChannel(uint8_t v) { return v * 2; }

#if MSS_MCPWM_VOICES > 0
static mcpwm_unit_t mcUnit(uint8_t m) { return m < 3 ? MCPWM_UNIT_0 : MCPWM_UNIT_1; }
static mcpwm_timer_t mcTimer(uint8_t m) { return (mcpwm_timer_t)(m % 3); }
static const mcpwm_io_signals_t MC_IO[3] = {MCPWM0A, MCPWM1A, MCPWM2A};
#endif

uint8_t toneMaxVoices() { return MSS_LEDC_VOICES + MSS_MCPWM_VOICES; }

bool toneBegin(const uint8_t *pins, uint8_t count) {
  if (count > toneMaxVoices()) return false;
  pins_ = pins;
  for (uint8_t v = 0; v < count; v++) {
    if (v < MSS_LEDC_VOICES) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
      if (!ledcAttachChannel(pins[v], 1000, 10, ledcChannel(v))) return false;
      ledcWrite(pins[v], 0);
#else
      ledcSetup(ledcChannel(v), 1000, 10);
      ledcAttachPin(pins[v], ledcChannel(v));
      ledcWrite(ledcChannel(v), 0);
#endif
    }
#if MSS_MCPWM_VOICES > 0
    else {
      uint8_t m = v - MSS_LEDC_VOICES;
      mcpwm_config_t cfg = {};
      cfg.frequency = 1000;
      cfg.cmpr_a = 0;
      cfg.cmpr_b = 0;
      cfg.duty_mode = MCPWM_DUTY_MODE_0;
      cfg.counter_mode = MCPWM_UP_COUNTER;
      mcpwm_gpio_init(mcUnit(m), MC_IO[m % 3], pins[v]);
      mcpwm_init(mcUnit(m), mcTimer(m), &cfg);
      mcpwm_set_signal_low(mcUnit(m), mcTimer(m), MCPWM_OPR_A);
    }
#endif
  }
  return true;
}

void toneSet(uint8_t v, float hz) {
  if (v < MSS_LEDC_VOICES) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWriteTone(pins_[v], hz);
#else
    ledcWriteTone(ledcChannel(v), hz);
#endif
  }
#if MSS_MCPWM_VOICES > 0
  else {
    uint8_t m = v - MSS_LEDC_VOICES;
    mcpwm_set_frequency(mcUnit(m), mcTimer(m), (uint32_t)hz);
    mcpwm_set_duty(mcUnit(m), mcTimer(m), MCPWM_OPR_A, 50.0f);
    mcpwm_set_duty_type(mcUnit(m), mcTimer(m), MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
  }
#endif
}

void toneStop(uint8_t v) {
  if (v < MSS_LEDC_VOICES) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWriteTone(pins_[v], 0);
#else
    ledcWriteTone(ledcChannel(v), 0);
#endif
  }
#if MSS_MCPWM_VOICES > 0
  else {
    uint8_t m = v - MSS_LEDC_VOICES;
    mcpwm_set_signal_low(mcUnit(m), mcTimer(m), MCPWM_OPR_A);
  }
#endif
}

} // namespace mss

#endif // ARDUINO_ARCH_ESP32
