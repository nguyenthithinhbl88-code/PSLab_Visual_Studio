#include "xa_audio.hpp"
#include <algorithm>
#include <vector>
namespace ps1 {
bool XaAudio::decode(const std::array<uint8_t,2352>& raw){
 const unsigned coding=raw[19];
 if((coding&3)>1||(coding&0x30)>0x10||(coding&0x88)){invalid++;return false;}
 const bool stereo=coding&1,eight=coding&16,low_rate=coding&4;
 std::array<std::vector<int16_t>,2> decoded;
 constexpr int positive[]={0,60,115,98},negative[]={0,0,-52,-55};
 for(unsigned group=0;group<18;group++){
  const unsigned base=24+group*128;
  for(unsigned unit=0;unit<(eight?4u:8u);unit++){
   const unsigned ch=stereo?(unit&1):0,header=raw[base+4+unit];
   unsigned shift=header&15;if(shift>12)shift=9;
   const unsigned filter=(header>>4)&3;
   for(unsigned n=0;n<28;n++){
    const unsigned packed=raw[base+16+n*4+(eight?unit:unit/2)];
    int sample=eight?int(int8_t(packed)):int((packed>>((unit&1)*4))&15);
    if(!eight&&sample>=8)sample-=16;
    sample=(sample*(eight?256:4096))>>shift;
    sample=std::clamp(sample+((previous[ch]*positive[filter]+older[ch]*negative[filter]+32)>>6),-32768,32767);
    older[ch]=previous[ch];previous[ch]=sample;decoded[ch].push_back(int16_t(sample));
   }
  }
 }
 if(!stereo)decoded[1]=decoded[0];
 // Functional resampling; the hardware's 25-point zigzag/emphasis filter is not modeled.
 const unsigned input_count=unsigned(decoded[0].size());
 const unsigned numerator=low_rate?3:6,output_count=input_count*7/numerator;
 for(unsigned n=0;n<output_count;n++){
  const unsigned at=n*numerator,i=at/7,f=at%7,j=std::min(i+1,input_count-1);
  std::array<int16_t,2> frame{};
  for(unsigned ch=0;ch<2;ch++)frame[ch]=int16_t((int(decoded[ch][i])*int(7-f)+int(decoded[ch][j])*int(f))/7);
  if(pcm.size()>=22050){pcm.pop_front();dropped++;}
  pcm.push_back(frame);
 }
 sectors++;frames+=output_count;return true;
}
}
