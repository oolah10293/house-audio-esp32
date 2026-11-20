#include "player.h"
#include "dsp_processor.h"
#include <cstdint>
int upstream_calls = 0;
int upstream_result = 0;
extern "C" int dsp_processor_worker(void *pcm_chunk, const void *) {
  ++upstream_calls;
  if (upstream_result != 0) return upstream_result;
  auto *chunk = static_cast<pcm_chunk_message_t *>(pcm_chunk);
  auto *frames = reinterpret_cast<uint32_t *>(chunk->fragment->payload);
  for (size_t i = 0; i < chunk->fragment->size / 4; ++i) {
    const int16_t l = static_cast<int16_t>(frames[i]);
    const int16_t r = static_cast<int16_t>(frames[i] >> 16);
    frames[i] = static_cast<uint16_t>(l / 2) | (static_cast<uint32_t>(static_cast<uint16_t>(r / 2)) << 16);
  }
  return 0;
}
