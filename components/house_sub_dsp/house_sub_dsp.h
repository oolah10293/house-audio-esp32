#pragma once
#include <atomic>
#include "esphome/core/component.h"
#include "sub_filter.h"

namespace esphome {
namespace house_sub_dsp {

class HouseSubDSP : public Component {
 public:
  void set_lowpass_hz(float hz) { cutoff_hz_ = hz; filter_.set_cutoff(hz); }
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return 900.0f; }
  bool process(char *audio, size_t bytes, uint32_t sample_rate);
  void reset_after_error() { filter_.reset(); bad_chunks_.fetch_add(1, std::memory_order_relaxed); }

 protected:
  MonoLowPass filter_;
  float cutoff_hz_{90.0f};
  uint32_t last_pcm_ms_{0};
  bool seen_audio_{false};
  // The audio task publishes only counters; formatted logging stays in loop().
  std::atomic<uint32_t> audio_rate_{0};
  std::atomic<uint32_t> bad_chunks_{0};
  uint32_t logged_rate_{0};
};

extern HouseSubDSP *active_sub_dsp;

}  // namespace house_sub_dsp
}  // namespace esphome
