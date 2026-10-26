#include "audio_resampler.hpp"
#include <algorithm>
#include <cmath>
namespace ps1 {
bool AudioResampler::fill(std::deque<int16_t>& input,int16_t* output,unsigned frames,double ratio,bool muted){
 if(!frames)return true;
 if(!std::isfinite(ratio)||ratio<0.25||ratio>2.0)return false;
 const double end=fraction+frames*ratio;
 const unsigned consumed=unsigned(end),last=unsigned(fraction+(frames-1)*ratio);
 if(input.size()/2<std::max(consumed,last+2))return false;
 for(unsigned n=0;n<frames;n++){
  const double at=fraction+n*ratio;const unsigned i=unsigned(at);const double f=at-i;
  for(unsigned ch=0;ch<2;ch++){
   const double value=input[2*i+ch]*(1-f)+input[2*(i+1)+ch]*f;
   output[2*n+ch]=muted?0:int16_t(std::clamp(int(std::lround(value)),-32768,32767));
  }
 }
 for(unsigned n=0;n<consumed*2;n++)input.pop_front();
 fraction=end-consumed;return true;
}
}
