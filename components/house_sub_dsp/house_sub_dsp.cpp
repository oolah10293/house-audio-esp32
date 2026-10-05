#include "house_sub_dsp.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "sdkconfig.h"

#if !CONFIG_USE_DSP_PROCESSOR || !CONFIG_SNAPCLIENT_DSP_FLOW_STEREO || !CONFIG_SNAPCLIENT_USE_SOFT_VOL
#error "house_sub_dsp requires the original Snapclient stereo/software-volume DSP configuration"
#endif

namespace esphome {
namespace house_sub_dsp {

static const char *const TAG = "house_sub_dsp";
HouseSubDSP *active_sub_dsp = nullptr;

void HouseSubDSP::setup() {
  active_sub_dsp = this;
  ESP_LOGI(TAG, "Configured: mono (L+R)/2, %.1f Hz LR4 low-pass, both DAC channels", cutoff_hz_);
}

void HouseSubDSP::dump_config() {
  ESP_LOGCONFIG(TAG, "Subwoofer DSP: mono, %.1f Hz, 24 dB/octave; no added audio buffer", cutoff_hz_);
}

bool HouseSubDSP::process(char *audio, size_t bytes, uint32_t sample_rate) {
  if (bytes == 0) return true;
  const uint32_t now = millis();
  // Do not replay the previous filter tail after a long pause/reconnect.
  if (seen_audio_ && static_cast<uint32_t>(now - last_pcm_ms_) > 500U) filter_.reset();
  last_pcm_ms_ = now;
  if (!filter_.process(audio, bytes, sample_rate)) {
    ESP_LOGE(TAG, "Invalid PCM buffer/rate: expected aligned 16-bit stereo");
    return false;
  }
  if (!seen_audio_ || logged_rate_ != sample_rate) {
    ESP_LOGI(TAG, "PCM DSP ACTIVE: mono, %.1f Hz LR4, %u Hz stream", cutoff_hz_, static_cast<unsigned>(sample_rate));
    logged_rate_ = sample_rate;
  }
  seen_audio_ = true;
  return true;
}

}  // namespace house_sub_dsp
}  // namespace esphome

// Signature matches the upstream C DSP entry point exactly. The linker redirects
// calls from decoder.cpp here; __real resolves to the untouched upstream worker.
extern "C" int __real_dsp_processor_worker(char *audio, size_t chunk_size, uint32_t samplerate);
extern "C" int __wrap_dsp_processor_worker(char *audio, size_t chunk_size, uint32_t samplerate) {
  // Preserve the current Snapcast software-volume handling, exactly once.
  const int result = __real_dsp_processor_worker(audio, chunk_size, samplerate);
  if (result != 0) return result;
  auto *dsp = esphome::house_sub_dsp::active_sub_dsp;
  if (dsp != nullptr && dsp->process(audio, chunk_size, samplerate)) return 0;
  // A configuration error must not silently turn this into a full-range output.
  if (audio != nullptr && (reinterpret_cast<uintptr_t>(audio) & 3U) == 0) {
    volatile uint32_t *frames = reinterpret_cast<volatile uint32_t *>(audio);
    for (size_t i = 0; i < chunk_size / 4U; ++i) frames[i] = 0;
  }
  return -1;
}
