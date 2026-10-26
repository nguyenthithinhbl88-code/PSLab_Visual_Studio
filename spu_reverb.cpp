#include "spu.hpp"
#include <algorithm>
namespace ps1 {
void Spu::reverb(int left,int right){
 // Documented IIR/COMB/APF network at 22050 Hz. The hardware 39-tap resampler
 // is approximated with pair averaging and held output; this is not bit-exact.
 auto clamp=[](int n){return std::clamp(n,-32768,32767);};
 if(!reverb_half){send_left=clamp(left);send_right=clamp(right);reverb_half=true;return;}
 reverb_half=false;left=(clamp(left)+send_left)/2;right=(clamp(right)+send_right)/2;
 if(!(regs[0x1ac/2]&12)){reverb_left=reverb_right=0;return;}
 const uint32_t base=uint32_t(regs[0x1a2/2])*8;
 const uint32_t size=0x80000-base;
 if(reverb_cursor<base||reverb_cursor>=0x80000)reverb_cursor=base;
 auto reg=[this](unsigned i){return regs[0x1c0/2+i];};
 auto volume=[&](unsigned i){return int16_t(reg(i));};
 auto multiply=[](int n,int coefficient){return int((int64_t(n)*coefficient)>>15);};
 auto addr=[&](int offset){int64_t a=int64_t(reverb_cursor-base)+offset;a%=size;if(a<0)a+=size;return base+uint32_t(a);};
 auto read=[&](int offset){const auto a=addr(offset);check_irq(a);return int(int16_t(ram[a/2]));};
 const bool write_enabled=regs[0x1aa/2]&128;
 auto write=[&](int offset,int sample){if(write_enabled){const auto a=addr(offset);check_irq(a);ram[a/2]=uint16_t(clamp(sample));}};
 auto offset=[&](unsigned i){return int(reg(i))*8;};
 const int input_l=multiply(left,volume(30)),input_r=multiply(right,volume(31));
 if(write_enabled){
  for(unsigned channel=0;channel<2;channel++){
   const int in=channel?input_r:input_l;
   const int same=offset(10+channel),different=offset(18+channel);
   const int old_same=read(same-2),old_different=read(different-2);
   int same_value=old_same+multiply(in+multiply(read(offset(16+channel)),volume(7))-old_same,volume(2));
   int diff_value=old_different+multiply(in+multiply(read(offset(24+(1-channel))),volume(7))-old_different,volume(2));
   if(volume(2)==-32768){same_value=-same_value;diff_value=-diff_value;}
   write(same,same_value);write(different,diff_value);
  }
 }
 int output[2]{};
 for(unsigned channel=0;channel<2;channel++){
  int out=0;
  const unsigned comb[]={12,14,20,22};
  for(unsigned i=0;i<4;i++)out+=multiply(read(offset(comb[i]+channel)),volume(3+i));
  out=clamp(out);
  for(unsigned stage=0;stage<2;stage++){
   const int destination=offset(26+stage*2+channel);
   const int delayed=read(destination-offset(stage));
   out=clamp(out-multiply(delayed,volume(8+stage)));write(destination,out);
   out=clamp(multiply(out,volume(8+stage))+delayed);
  }
  output[channel]=multiply(out,int16_t(regs[0x184/2+channel]));
 }
 reverb_left=output[0];reverb_right=output[1];
 reverb_cursor+=2;if(reverb_cursor>=0x80000)reverb_cursor=base;
}
}
