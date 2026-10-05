#include <cstdint>
#include <cstddef>
int upstream_calls=0;
int upstream_result=0;
extern "C" int dsp_processor_worker(char *audio,size_t n,uint32_t) {
  ++upstream_calls;
  if (upstream_result) return upstream_result;
  auto *p=reinterpret_cast<uint32_t*>(audio);
  for (size_t i=0;i<n/4;++i) {
    int16_t a=int16_t(p[i]), b=int16_t(p[i]>>16);
    p[i]=uint16_t(a/2) | (uint32_t(uint16_t(b/2))<<16);
  }
  return 0;
}
