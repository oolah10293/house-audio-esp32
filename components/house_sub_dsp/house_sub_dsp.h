#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "esphome/core/component.h"
#include "sub_filter.h"

namespace esphome {
namespace house_sub_dsp {

class HouseSubDSP : public Component {
 public:
  void set_lowpass_hz(float hz);
  void set_lowcut_hz(float hz);
  void set_phase_inverted(bool inverted);
  void set_crossover_bypass(bool bypass);

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return 900.0f; }

  bool process(char *audio, size_t bytes, uint32_t sample_rate);
  void reset_after_error() {
    filter_.reset();
    bad_chunks_.fetch_add(1, std::memory_order_relaxed);
  }

 protected:
  static uint32_t hz_to_millihz_(float hz);
  void apply_requested_settings_();

  MonoCrossover filter_;
  std::atomic<uint32_t> requested_lowpass_millihz_{90000};
  std::atomic<uint32_t> requested_lowcut_millihz_{0};
  std::atomic<bool> requested_phase_inverted_{false};
  std::atomic<bool> requested_crossover_bypass_{false};
  std::atomic<uint32_t> requested_version_{1};

  uint32_t active_version_{0};
  uint32_t active_lowpass_millihz_{90000};
  uint32_t active_lowcut_millihz_{0};
  bool active_phase_inverted_{false};
  bool active_crossover_bypass_{false};

  std::atomic<uint32_t> applied_version_{0};
  std::atomic<uint32_t> audio_rate_{0};
  std::atomic<uint32_t> bad_chunks_{0};
  uint32_t logged_version_{0};
  uint32_t logged_rate_{0};
  uint32_t last_pcm_ms_{0};
  bool seen_audio_{false};
};

extern HouseSubDSP *active_sub_dsp;

}  // namespace house_sub_dsp
}  // namespace esphome
