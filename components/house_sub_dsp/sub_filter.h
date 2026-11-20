#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace esphome {
namespace house_sub_dsp {

// 16-bit interleaved stereo, one aligned 32-bit word per frame.
// Audio is always summed to mono and copied to both output slots.
class MonoCrossover {
 public:
  void set_settings(float lowpass_hz, float lowcut_hz, bool phase_inverted,
                    bool crossover_bypass) {
    if (same_(lowpass_hz_, lowpass_hz) && same_(lowcut_hz_, lowcut_hz) &&
        phase_inverted_ == phase_inverted && crossover_bypass_ == crossover_bypass)
      return;
    lowpass_hz_ = lowpass_hz;
    lowcut_hz_ = lowcut_hz;
    phase_inverted_ = phase_inverted;
    crossover_bypass_ = crossover_bypass;
    sample_rate_ = 0;
    reset();
  }

  void reset() {
    for (unsigned i = 0; i < 2; ++i) {
      lp_state_[i] = {};
      hp_state_[i] = {};
    }
  }

  bool prepare(uint32_t sample_rate) {
    if (!valid_settings_(sample_rate)) return false;
    configure_(sample_rate);
    return true;
  }

  bool process(char *audio, size_t bytes, uint32_t sample_rate) {
    if (audio == nullptr || (reinterpret_cast<uintptr_t>(audio) & 3U) != 0 ||
        bytes % 4U != 0 || !valid_settings_(sample_rate))
      return false;
    if (sample_rate != sample_rate_) configure_(sample_rate);

    volatile uint32_t *frames = reinterpret_cast<volatile uint32_t *>(audio);
    for (size_t i = 0; i < bytes / 4U; ++i) {
      const uint32_t packed = frames[i];
      const int32_t left = static_cast<int16_t>(packed & 0xFFFFU);
      const int32_t right = static_cast<int16_t>(packed >> 16);
      // Widen before adding. /65536 is average-and-normalize for int16.
      float y = static_cast<float>(left + right) * (1.0f / 65536.0f);

      // Bypass means crossover bypass; mono summing and phase remain active.
      if (!crossover_bypass_) {
        if (lowcut_hz_ > 0.0f) {
          y = run_(y, hp_coeffs_, hp_state_[0]);
          y = run_(y, hp_coeffs_, hp_state_[1]);
        }
        y = run_(y, lp_coeffs_, lp_state_[0]);
        y = run_(y, lp_coeffs_, lp_state_[1]);
      }
      if (phase_inverted_) y = -y;

      if (!std::isfinite(y)) {
        reset();
        y = 0.0f;
      }
      if (y > 32767.0f / 32768.0f) y = 32767.0f / 32768.0f;
      if (y < -1.0f) y = -1.0f;
      const int16_t pcm = static_cast<int16_t>(static_cast<int32_t>(y * 32768.0f));
      const uint32_t bits = static_cast<uint16_t>(pcm);
      frames[i] = bits | (bits << 16);
    }
    return true;
  }

 private:
  struct Coeffs {
    float b0{0.0f}, b1{0.0f}, b2{0.0f}, a1{0.0f}, a2{0.0f};
  };
  struct State {
    float z1{0.0f}, z2{0.0f};
  };

  static bool same_(float a, float b) { return std::fabs(a - b) < 0.001f; }

  bool valid_settings_(uint32_t sample_rate) const {
    if (sample_rate < 8000U || sample_rate > 192000U ||
        !std::isfinite(lowpass_hz_) || !std::isfinite(lowcut_hz_) ||
        lowpass_hz_ < 20.0f || lowpass_hz_ >= 0.45f * sample_rate ||
        lowcut_hz_ < 0.0f ||
        (lowcut_hz_ > 0.0f &&
         (lowcut_hz_ < 10.0f || lowcut_hz_ >= lowpass_hz_ ||
          lowcut_hz_ >= 0.45f * sample_rate)))
      return false;
    return true;
  }

  static Coeffs make_lowpass_(float hz, uint32_t sample_rate) {
    constexpr double PI = 3.14159265358979323846;
    const double w = 2.0 * PI * hz / sample_rate;
    const double c = std::cos(w);
    const double alpha = std::sin(w) * 0.70710678118654752440;
    const double a0 = 1.0 + alpha;
    Coeffs out;
    out.b0 = static_cast<float>((1.0 - c) * 0.5 / a0);
    out.b1 = 2.0f * out.b0;
    out.b2 = out.b0;
    out.a1 = static_cast<float>(-2.0 * c / a0);
    out.a2 = static_cast<float>((1.0 - alpha) / a0);
    return out;
  }

  static Coeffs make_highpass_(float hz, uint32_t sample_rate) {
    constexpr double PI = 3.14159265358979323846;
    const double w = 2.0 * PI * hz / sample_rate;
    const double c = std::cos(w);
    const double alpha = std::sin(w) * 0.70710678118654752440;
    const double a0 = 1.0 + alpha;
    Coeffs out;
    out.b0 = static_cast<float>((1.0 + c) * 0.5 / a0);
    out.b1 = -2.0f * out.b0;
    out.b2 = out.b0;
    out.a1 = static_cast<float>(-2.0 * c / a0);
    out.a2 = static_cast<float>((1.0 - alpha) / a0);
    return out;
  }

  static float run_(float x, const Coeffs &c, State &s) {
    const float y = c.b0 * x + s.z1;
    s.z1 = c.b1 * x - c.a1 * y + s.z2;
    s.z2 = c.b2 * x - c.a2 * y;
    return y;
  }

  void configure_(uint32_t sample_rate) {
    lp_coeffs_ = make_lowpass_(lowpass_hz_, sample_rate);
    if (lowcut_hz_ > 0.0f) hp_coeffs_ = make_highpass_(lowcut_hz_, sample_rate);
    sample_rate_ = sample_rate;
    reset();
  }

  float lowpass_hz_{90.0f};
  float lowcut_hz_{0.0f};
  bool phase_inverted_{false};
  bool crossover_bypass_{false};
  uint32_t sample_rate_{0};
  Coeffs lp_coeffs_{};
  Coeffs hp_coeffs_{};
  State lp_state_[2]{};
  State hp_state_[2]{};
};

}  // namespace house_sub_dsp
}  // namespace esphome
