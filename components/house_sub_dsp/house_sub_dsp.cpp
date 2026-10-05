#include "house_sub_dsp.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "sdkconfig.h"
#include <type_traits>

// Use the real headers from PR #14389's pinned core. A stale raw-buffer ABI
// cannot compile against this adapter.
#include "player.h"
#include "dsp_processor.h"

#if !CONFIG_USE_DSP_PROCESSOR || !CONFIG_SNAPCLIENT_USE_SOFT_VOL
#error "house_sub_dsp requires PR #14389's software-volume DSP configuration"
#endif

static_assert(std::is_same<decltype(&dsp_processor_worker), int (*)(void *, const void *)>::value,
              "Unexpected Snapclient DSP ABI: expected PR #14389 PCM chunk/settings");

namespace esphome {
namespace house_sub_dsp {
static const char *const TAG = "house_sub_dsp";
HouseSubDSP *active_sub_dsp = nullptr;

void HouseSubDSP::setup() {
  // Calculate the normal HOUSE 48 kHz coefficients outside the audio task.
  alignas(4) uint32_t silence = 0;
  filter_.process(reinterpret_cast<char *>(&silence), sizeof(silence), 48000);
  filter_.reset();
  active_sub_dsp = this;
  ESP_LOGI(TAG, "PR14389 adapter ready: mono (L+R)/2, %.1f Hz LR4", cutoff_hz_);
}

void HouseSubDSP::loop() {
  const uint32_t rate = audio_rate_.load(std::memory_order_relaxed);
  if (rate != 0 && rate != logged_rate_) {
    ESP_LOGI(TAG, "PCM DSP ACTIVE: PR14389, mono, %.1f Hz LR4, %u Hz stream", cutoff_hz_, static_cast<unsigned>(rate));
    logged_rate_ = rate;
  }
  if (bad_chunks_.exchange(0, std::memory_order_relaxed) != 0) {
    ESP_LOGE(TAG, "Rejected PCM chunk: expected valid 16-bit stereo; output silenced");
  }
}

void HouseSubDSP::dump_config() {
  ESP_LOGCONFIG(TAG, "Sub DSP: PR14389 core 7742680; mono, %.1f Hz LR4; both DAC outputs", cutoff_hz_);
}

bool HouseSubDSP::process(char *audio, size_t bytes, uint32_t sample_rate) {
  if (bytes == 0) return true;
  const uint32_t now = millis();
  if (seen_audio_ && static_cast<uint32_t>(now - last_pcm_ms_) > 500U) filter_.reset();
  last_pcm_ms_ = now;
  if (!filter_.process(audio, bytes, sample_rate)) return false;
  audio_rate_.store(sample_rate, std::memory_order_relaxed);
  seen_audio_ = true;
  return true;
}
}  // namespace house_sub_dsp
}  // namespace esphome

namespace {
bool valid_chunk(const pcm_chunk_message_t *chunk, const snapcastSetting_t *settings) {
  if (chunk == nullptr || settings == nullptr || chunk->fragment == nullptr ||
      settings->ch != 2 || static_cast<unsigned>(settings->bits) != 16 ||
      settings->sr < 8000 || settings->sr > 192000 || chunk->totalSize == 0) return false;
  size_t total = 0;
  for (auto *f = chunk->fragment; f != nullptr; f = f->nextFragment) {
    if (f->payload == nullptr || (reinterpret_cast<uintptr_t>(f->payload) & 3U) != 0 ||
        f->size == 0 || f->size % 4U != 0 || f->size / 4U > 32767U ||
        f->size > chunk->totalSize - total) return false;
    total += f->size;
  }
  return total == chunk->totalSize;
}

void silence_chunk(pcm_chunk_message_t *chunk) {
  if (chunk == nullptr) return;
  size_t remaining = chunk->totalSize;
  for (auto *f = chunk->fragment; f != nullptr && remaining != 0; f = f->nextFragment) {
    const size_t bytes = f->size < remaining ? f->size : remaining;
    if (bytes == 0) break;
    if (f->payload != nullptr && (reinterpret_cast<uintptr_t>(f->payload) & 3U) == 0) {
      volatile uint32_t *frames = reinterpret_cast<volatile uint32_t *>(f->payload);
      for (size_t i = 0; i < bytes / 4U; ++i) frames[i] = 0;
    }
    remaining -= bytes;
  }
}
}

extern "C" int __real_dsp_processor_worker(void *pcm_chunk, const void *settings);
extern "C" int __wrap_dsp_processor_worker(void *pcm_chunk, const void *settings) {
  auto *chunk = static_cast<pcm_chunk_message_t *>(pcm_chunk);
  const auto *sc = static_cast<const snapcastSetting_t *>(settings);
  auto *dsp = esphome::house_sub_dsp::active_sub_dsp;
  if (dsp == nullptr || !valid_chunk(chunk, sc)) {
    silence_chunk(chunk);
    if (dsp != nullptr) dsp->reset_after_error();
    return -1;
  }
  // Upstream's worker handles one fragment. Give it a temporary view of each
  // fragment, retaining software volume exactly once per sample. The original
  // chunk's timestamps, size, fragment links and queue ownership never change.
  for (auto *f = chunk->fragment; f != nullptr; f = f->nextFragment) {
    pcm_chunk_fragment_t fragment_view = *f;
    fragment_view.nextFragment = nullptr;
    pcm_chunk_message_t chunk_view = *chunk;
    chunk_view.fragment = &fragment_view;
    chunk_view.totalSize = f->size;
    const int result = __real_dsp_processor_worker(&chunk_view, sc);
    if (result != 0 || !dsp->process(f->payload, f->size, static_cast<uint32_t>(sc->sr))) {
      silence_chunk(chunk);
      dsp->reset_after_error();
      return result != 0 ? result : -1;
    }
  }
  return 0;
}
