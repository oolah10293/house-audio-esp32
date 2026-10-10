#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "player.h"
#include "dsp_processor.h"
#include "../components/house_sub_dsp/house_sub_dsp.h"
extern int upstream_calls;
extern int upstream_result;
static uint32_t pack(int16_t a,int16_t b){return static_cast<uint16_t>(a)|(static_cast<uint32_t>(static_cast<uint16_t>(b))<<16);}
int main(){
  using namespace esphome::house_sub_dsp;
  HouseSubDSP dsp;dsp.set_lowpass_hz(90);dsp.setup();
  snapcastSetting_t s{};s.sr=48000;s.ch=2;s.bits=16;
  std::vector<uint32_t> frames(4800,pack(10000,0));
  pcm_chunk_fragment_t f{frames.size()*4,reinterpret_cast<char*>(frames.data()),nullptr};
  pcm_chunk_message_t c{{1,2},f.size,&f,0};
  assert(dsp_processor_worker(&c,&s)==0);assert(upstream_calls==1);
  int16_t baseline=static_cast<int16_t>(frames.back());assert(baseline>2490&&baseline<2510);
  dsp.set_phase_inverted(true);std::fill(frames.begin(),frames.end(),pack(10000,0));assert(dsp_processor_worker(&c,&s)==0);assert(static_cast<int16_t>(frames.back())==-baseline);
  dsp.set_phase_inverted(false);dsp.set_crossover_bypass(true);std::fill(frames.begin(),frames.end(),pack(10000,0));assert(dsp_processor_worker(&c,&s)==0);assert(static_cast<int16_t>(frames.back())==2500);
  dsp.set_crossover_bypass(false);dsp.set_lowcut_hz(30);dsp.set_lowpass_hz(120);std::fill(frames.begin(),frames.end(),pack(10000,10000));assert(dsp_processor_worker(&c,&s)==0);
  for(auto x:frames)assert(static_cast<uint16_t>(x)==static_cast<uint16_t>(x>>16));
  s.bits=24;int calls=upstream_calls;assert(dsp_processor_worker(&c,&s)==-1);assert(upstream_calls==calls);for(auto x:frames)assert(x==0);s.bits=16;
  upstream_result=-7;std::fill(frames.begin(),frames.end(),pack(10000,10000));assert(dsp_processor_worker(&c,&s)==-7);for(auto x:frames)assert(x==0);upstream_result=0;
  std::puts("PASS: runtime controls, PR14389 ABI, mono slots, single volume and fail-closed guards");
}
