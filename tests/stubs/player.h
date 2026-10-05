#pragma once
#include <cstddef>
#include <cstdint>
struct pcm_chunk_fragment {
  size_t size;
  char *payload;
  pcm_chunk_fragment *nextFragment;
};
using pcm_chunk_fragment_t = pcm_chunk_fragment;
struct tv_t { uint32_t sec; uint32_t usec; };
struct pcm_chunk_message_t {
  tv_t timestamp;
  size_t totalSize;
  pcm_chunk_fragment_t *fragment;
  uint32_t caps;
};
struct snapcastSetting_t {
  uint32_t buf_ms;
  uint32_t chkInFrames;
  int32_t cDacLat_ms;
  int codec;
  int32_t sr;
  uint8_t ch;
  int bits;
  bool muted;
  char *pcmBuf;
  uint32_t pcmBufSize;
};
