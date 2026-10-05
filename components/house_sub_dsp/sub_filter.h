#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace esphome {
namespace house_sub_dsp {

// 16-bit interleaved stereo, one aligned 32-bit word per frame, as passed by
// c-MM/esphome-snapclient ce51e2fd... to dsp_processor_worker().
// Both output slots receive the same (L+R)/2 -> LR4 low-pass result.
class MonoLowPass {
 public:
  void set_cutoff(float hz) { cutoff_hz_ = hz; sample_rate_ = 0; reset(); }
  void reset() { z1_[0] = z1_[1] = z2_[0] = z2_[1] = 0.0f; }

  bool process(char *audio, size_t bytes, uint32_t sample_rate) {
    if (audio == nullptr || (reinterpret_cast<uintptr_t>(audio) & 3U) != 0 ||
        bytes % 4U != 0 || sample_rate < 8000U || sample_rate > 192000U ||
        !std::isfinite(cutoff_hz_) || cutoff_hz_ <= 0.0f ||
        cutoff_hz_ >= 0.45f * static_cast<float>(sample_rate)) {
      return false;
    }
    if (sample_rate != sample_rate_) {
      configure_(sample_rate);
    }
    // Match upstream's 32-bit-only access requirement for audio allocations.
    volatile uint32_t *frames = reinterpret_cast<volatile uint32_t *>(audio);
    for (size_t i = 0; i < bytes / 4U; ++i) {
      const uint32_t packed = frames[i];
      const int32_t a = static_cast<int16_t>(packed & 0xFFFFU);
      const int32_t b = static_cast<int16_t>(packed >> 16);
      // Widen before adding. Division by 2 prevents summing overflow.
      float y = static_cast<float>(a + b) * (1.0f / 65536.0f);
      for (unsigned stage = 0; stage < 2; ++stage) {
        const float x = y;
        y = b0_ * x + z1_[stage];
        z1_[stage] = b1_ * x - a1_ * y + z2_[stage];
        z2_[stage] = b0_ * x - a2_ * y;
      }
      if (!std::isfinite(y)) {
        reset();
        y = 0.0f;
      }
      // A real low-pass can overshoot on transients: saturate, never wrap.
      if (y > 32767.0f / 32768.0f) y = 32767.0f / 32768.0f;
      if (y < -1.0f) y = -1.0f;
      const int16_t pcm = static_cast<int16_t>(static_cast<int32_t>(y * 32768.0f));
      const uint32_t bits = static_cast<uint16_t>(pcm);
      frames[i] = bits | (bits << 16);
    }
    return true;
  }

 private:
  void configure_(uint32_t sample_rate) {
    // Two cascaded second-order Butterworth sections, each Q=1/sqrt(2).
    // Together: Linkwitz-Riley fourth order, -6.02 dB at cutoff, 24 dB/oct.
    // Compute coefficients in double once, process every sample in float.
    constexpr double PI = 3.14159265358979323846;
    const double w = 2.0 * PI * static_cast<double>(cutoff_hz_) / sample_rate;
    const double c = std::cos(w);
    const double alpha = std::sin(w) * 0.70710678118654752440;
    const double a0 = 1.0 + alpha;
    b0_ = static_cast<float>((1.0 - c) * 0.5 / a0);
    b1_ = 2.0f * b0_;
    a1_ = static_cast<float>(-2.0 * c / a0);
    a2_ = static_cast<float>((1.0 - alpha) / a0);
    sample_rate_ = sample_rate;
    reset();
  }

  float cutoff_hz_{90.0f};
  uint32_t sample_rate_{0};
  float b0_{0.0f}, b1_{0.0f}, a1_{0.0f}, a2_{0.0f};
  float z1_[2]{0.0f, 0.0f}, z2_[2]{0.0f, 0.0f};
};

}  // namespace house_sub_dsp
}  // namespace esphome
