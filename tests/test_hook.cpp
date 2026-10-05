#include "../components/house_sub_dsp/house_sub_dsp.h"
#include "esphome/core/hal.h"
#include <cassert>
#include <vector>
#include <cstdio>
extern int upstream_calls, upstream_result;
extern "C" int dsp_processor_worker(char*,size_t,uint32_t);
int main() {
  esphome::house_sub_dsp::HouseSubDSP dsp;
  dsp.set_lowpass_hz(90);dsp.setup();dsp.dump_config();
  std::vector<uint32_t> v(48000, uint32_t(uint16_t(16000))<<16);
  assert(dsp_processor_worker(reinterpret_cast<char*>(v.data()),v.size()*4,48000)==0);
  assert(upstream_calls==1);
  // 16000 right only -> existing 50% volume -> mono /2 -> 4000.
  assert(std::abs(int(int16_t(v.back()))-4000)<8);
  for(auto w:v) assert(uint16_t(w)==uint16_t(w>>16));
  // A long stream gap clears the old filter tail before zero input.
  esphome::test_ms=1000;std::fill(v.begin(),v.end(),0);
  assert(dsp_processor_worker(reinterpret_cast<char*>(v.data()),v.size()*4,48000)==0);
  for(auto w:v) assert(w==0);
  upstream_result=-7;
  assert(dsp_processor_worker(reinterpret_cast<char*>(v.data()),v.size()*4,48000)==-7);
  assert(upstream_calls==3);
  std::puts("PASS: link wrapping, upstream volume called once, mono+filter active, gap reset, error propagation");
}
