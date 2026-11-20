#include "../components/house_sub_dsp/sub_filter.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using esphome::house_sub_dsp::MonoCrossover;
constexpr double PI = 3.14159265358979323846;
static uint32_t pack(int16_t a, int16_t b) { return uint32_t(uint16_t(a)) | (uint32_t(uint16_t(b)) << 16); }
static std::vector<uint32_t> tone(double hz, unsigned sr, int channel) {
  std::vector<uint32_t> v(sr);
  for (unsigned i=0; i<sr; ++i) {
    int16_t s = int16_t(12000.0 * std::sin(2.0*PI*hz*i/sr));
    v[i] = pack(channel == 1 ? 0 : s, channel == 0 ? 0 : (channel == 3 ? -s : s));
  }
  return v;
}
static void run(MonoCrossover &f, std::vector<uint32_t> &v, unsigned sr, size_t block=960) {
  for (size_t i=0; i<v.size(); i+=block) {
    size_t n=std::min(block,v.size()-i);
    assert(f.process(reinterpret_cast<char *>(v.data()+i),n*4,sr));
  }
  for (uint32_t v0:v) assert(uint16_t(v0)==uint16_t(v0>>16));
}
static double rms(const std::vector<uint32_t>& v) {
  double sum=0; size_t n=0;
  for (size_t i=v.size()/2;i<v.size();++i) { double y=int16_t(v[i]); sum+=y*y; ++n; }
  return std::sqrt(sum/n);
}
static double ideal_lp_db(double hz, double fc, unsigned sr) {
  double ratio=std::tan(PI*hz/sr)/std::tan(PI*fc/sr);
  return -20*std::log10(1+std::pow(ratio,4));
}
static double ideal_hp_db(double hz, double fc, unsigned sr) {
  double ratio=std::tan(PI*fc/sr)/std::tan(PI*hz/sr);
  return -20*std::log10(1+std::pow(ratio,4));
}
int main() {
  for (unsigned sr: {44100U,48000U}) {
    auto left=tone(60,sr,0), right=tone(60,sr,1), both=tone(60,sr,2), opposite=tone(60,sr,3);
    MonoCrossover l,r,b,o; run(l,left,sr);run(r,right,sr);run(b,both,sr);run(o,opposite,sr);
    assert(left==right);assert(std::abs(rms(both)/rms(left)-2.0)<0.003);assert(rms(opposite)==0.0);
    for (double hz:{30.0,45.0,90.0,180.0,360.0}) {
      auto v=tone(hz,sr,2); double input=rms(v); MonoCrossover f;run(f,v,sr);
      double db=20*std::log10(rms(v)/input), expected=ideal_lp_db(hz,90.0,sr);
      assert(std::abs(db-expected)<0.20);
    }
    for (double hz:{10.0,15.0,30.0,60.0,120.0}) {
      auto v=tone(hz,sr,2); double input=rms(v); MonoCrossover f;
      f.set_settings(160.0f,30.0f,false,false); run(f,v,sr);
      double db=20*std::log10(rms(v)/input);
      double expected=ideal_hp_db(hz,30.0,sr)+ideal_lp_db(hz,160.0,sr);
      assert(std::abs(db-expected)<0.25);
    }
    auto bypass=tone(300,sr,2); double input=rms(bypass); MonoCrossover by;
    by.set_settings(90.0f,30.0f,false,true); run(by,bypass,sr);assert(std::abs(rms(bypass)/input-1.0)<0.002);
    auto normal=tone(60,sr,2), inverted=normal; MonoCrossover n,inv;
    inv.set_settings(90.0f,0.0f,true,false); run(n,normal,sr); run(inv,inverted,sr);
    for (size_t i=sr/2;i<sr;++i) assert(std::abs(int16_t(normal[i])+int16_t(inverted[i]))<=1);
    auto one=tone(80,sr,2), split=one;MonoCrossover x,y;run(x,one,sr,sr);run(y,split,sr,137);assert(one==split);
  }
  MonoCrossover f;std::vector<uint32_t> silence(48000,0);run(f,silence,48000);assert(rms(silence)==0);
  for (int16_t sign:{int16_t(32767),int16_t(-32768)}) {
    std::vector<uint32_t> full(48000,pack(sign,sign));f.reset();run(f,full,48000);
    assert((int16_t(full.back())>0)==(sign>0));assert(std::abs(int(int16_t(full.back()))-int(sign))<40);
  }
  auto first=tone(60,48000,2);run(f,first,48000);f.reset();std::vector<uint32_t> zero(1000,0);run(f,zero,48000);assert(rms(zero)==0);
  auto changed=tone(70,44100,2), fresh=changed;MonoCrossover other;run(f,changed,44100);run(other,fresh,44100);assert(changed==fresh);
  auto live=tone(120,48000,2);MonoCrossover dyn;run(dyn,live,48000,48000);dyn.set_settings(160,0,false,false);
  auto after=tone(120,48000,2);run(dyn,after,48000,48000);assert(rms(after)>rms(live)*2.0);
  uint32_t sample=0;assert(!f.process(nullptr,4,48000));assert(!f.process(reinterpret_cast<char*>(&sample),3,48000));assert(!f.process(reinterpret_cast<char*>(&sample),4,0));
  f.set_settings(50,60,false,false);assert(!f.process(reinterpret_cast<char*>(&sample),4,48000));
  std::puts("PASS: mono sum, live LP/HP LR4, phase, bypass, outputs, boundaries and invalid input");
}
