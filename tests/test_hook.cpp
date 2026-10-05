#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "player.h"
#include "dsp_processor.h"
#include "../components/house_sub_dsp/house_sub_dsp.h"
extern int upstream_calls;
extern int upstream_result;
static uint32_t pack(int16_t a, int16_t b) { return static_cast<uint16_t>(a) | (static_cast<uint32_t>(static_cast<uint16_t>(b)) << 16); }
int main() {
  using namespace esphome::house_sub_dsp;
  HouseSubDSP dsp;
  dsp.set_lowpass_hz(90.0f);
  dsp.setup();
  snapcastSetting_t settings{};
  settings.sr = 48000; settings.ch = 2; settings.bits = 16;
  std::vector<uint32_t> frames(4800, pack(10000, 0));
  pcm_chunk_fragment_t fragment{frames.size() * 4, reinterpret_cast<char *>(frames.data()), nullptr};
  pcm_chunk_message_t chunk{{123, 456}, fragment.size, &fragment, 0};
  assert(dsp_processor_worker(&chunk, &settings) == 0);
  assert(upstream_calls == 1);
  const int left = static_cast<int16_t>(frames.back());
  assert(left > 2490 && left < 2510); // Half-volume upstream, then (L+R)/2 exactly once.
  for (auto x : frames) assert(static_cast<uint16_t>(x) == static_cast<uint16_t>(x >> 16));
  assert(chunk.timestamp.sec == 123 && chunk.timestamp.usec == 456);
  assert(chunk.totalSize == fragment.size && chunk.fragment == &fragment);
  assert(fragment.nextFragment == nullptr);
  dsp.loop();
  std::fill(frames.begin(), frames.end(), pack(0, 10000));
  dsp.reset_after_error();
  assert(dsp_processor_worker(&chunk, &settings) == 0);
  assert(static_cast<int16_t>(frames.back()) == left);
  // Fragment chain: all samples receive upstream volume and filtering once.
  std::vector<uint32_t> second(4800, pack(10000, 10000));
  pcm_chunk_fragment_t tail{second.size() * 4, reinterpret_cast<char *>(second.data()), nullptr};
  fragment.nextFragment = &tail;
  chunk.totalSize += tail.size;
  std::fill(frames.begin(), frames.end(), pack(10000, 10000));
  int calls = upstream_calls;
  assert(dsp_processor_worker(&chunk, &settings) == 0);
  assert(upstream_calls == calls + 2);
  assert(fragment.nextFragment == &tail && chunk.fragment == &fragment);
  assert(static_cast<int16_t>(second.back()) > 4980 && static_cast<int16_t>(second.back()) < 5020);
  // Unsupported formats fail closed without ever calling the upstream worker.
  settings.bits = 24; calls = upstream_calls;
  assert(dsp_processor_worker(&chunk, &settings) == -1);
  assert(upstream_calls == calls);
  for (auto x : frames) assert(x == 0);
  for (auto x : second) assert(x == 0);
  settings.bits = 16;
  assert(dsp_processor_worker(&chunk, nullptr) == -1);
  assert(dsp_processor_worker(nullptr, &settings) == -1);
  settings.ch = 1;
  assert(dsp_processor_worker(&chunk, &settings) == -1);
  settings.ch = 2;
  upstream_result = -7;
  std::fill(frames.begin(), frames.end(), pack(10000, 10000));
  assert(dsp_processor_worker(&chunk, &settings) == -7);
  for (auto x : frames) assert(x == 0);
  upstream_result = 0;
  active_sub_dsp = nullptr;
  assert(dsp_processor_worker(&chunk, &settings) == -1);
  std::puts("PASS: PR14389 two-argument ABI, mono slots, single volume application, fragments, timestamps, fail-closed guards");
}
