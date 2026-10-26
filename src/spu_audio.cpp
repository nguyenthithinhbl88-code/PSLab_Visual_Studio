#include "spu.hpp"
#include "spu_gauss.hpp"
#include <algorithm>
#include <cstdlib>
namespace ps1 {
namespace {
int clamp16(int n){return std::clamp(n,-32768,32767);}
}
int Spu::volume(unsigned slot,uint16_t value){
 auto& v=volumes[slot];
 if(!(value&0x8000)){v.level=((value&0x4000)?int(value&0x7fff)-0x8000:int(value&0x3fff))*2;v.counter=0;return v.level;}
 const unsigned shift=(value>>2)&31,step_bits=value&3;
 const bool decrease=value&0x2000,negative=value&0x1000,exponential=value&0x4000;
 int step=7-int(step_bits);if(decrease!=negative)step=~step;
 step*=1<<std::max(0,11-int(shift));
 unsigned increment=0x8000u>>std::max(0,int(shift)-11);
 if(exponential&&!decrease&&v.level>0x6000){if(shift<10)step/=4;else if(shift>=11)increment/=4;else{step/=2;increment/=2;}}
 else if(exponential&&decrease)step=int((int64_t(step)*std::abs(v.level))>>15);
 if(((shift<<2)|step_bits)!=127)increment=std::max(1u,increment);
 v.counter+=increment;
 if(v.counter&0x8000){v.counter&=0x7fff;
  if(decrease)v.level=negative?std::clamp(v.level+step,-32768,0):std::clamp(v.level+step,0,32767);
  else v.level=std::clamp(v.level+step,-32768,32767);
 }
 return v.level;
}
void Spu::check_irq(uint32_t address){
 if((regs[0x1aa/2]&0x8040)==0x8040&&((address&0x7fff8)==uint32_t(regs[0x1a4/2])*8))irq=true;
}
unsigned Spu::active_voices()const{unsigned n=0;for(const auto& v:voices)if(v.phase!=Voice::Off)n++;return n;}
void Spu::decode(unsigned n){
 auto& v=voices[n];
 auto byte=[this](uint32_t a){a&=0x7ffff;return uint8_t(ram[a/2]>>((a&1)*8));};
 check_irq(v.address);check_irq(v.address+8);
 const unsigned header=byte(v.address);unsigned shift=header&15,filter=header>>4;
 if(shift>12)shift=9;
 if(filter>4)filter=0;
 v.flags=byte(v.address+1);
 if(v.flags&4)regs[n*8+7]=uint16_t(v.address/8);
 constexpr int positive[]={0,60,115,98,122},negative[]={0,0,-52,-55,-60};
 for(unsigned i=0;i<3;i++)v.samples[i]=v.samples[28+i];
 for(unsigned i=0;i<28;i++){
  const unsigned packed=byte(v.address+2+i/2);int sample=int((packed>>((i&1)*4))&15);
  if(sample&8)sample-=16;
  sample=(sample*4096)>>shift;
  sample=clamp16(sample+((v.previous*positive[filter]+v.older*negative[filter]+32)>>6));
  v.older=v.previous;v.previous=sample;v.samples[i+3]=int16_t(sample);
 }
 v.decoded=true;
}
void Spu::envelope(unsigned n){
 auto& v=voices[n];const unsigned a=regs[n*8+4],b=regs[n*8+5];
 unsigned shift=0,step_bits=0;bool decrease=false,exponential=false;
 switch(v.phase){
 case Voice::Off:v.level=0;regs[n*8+6]=0;return;
 case Voice::Attack:shift=(a>>10)&31;step_bits=(a>>8)&3;exponential=a&0x8000;break;
 case Voice::Decay:shift=(a>>4)&15;decrease=exponential=true;break;
 case Voice::Sustain:shift=(b>>8)&31;step_bits=(b>>6)&3;decrease=b&0x4000;exponential=b&0x8000;break;
 case Voice::Release:shift=b&31;decrease=true;exponential=b&32;break;
 }
 int step=decrease?(-8+int(step_bits)):(7-int(step_bits));
 step*=1<<std::max(0,11-int(shift));
 unsigned increment=0x8000u>>std::max(0,int(shift)-11);
 if(exponential&&!decrease&&v.level>0x6000){
  if(shift<10)step/=4;else if(shift>=11)increment/=4;else{step/=2;increment/=2;}
 }else if(exponential&&decrease)step=(step*v.level)>>15;
 const bool frozen=(v.phase==Voice::Release?shift==31:((shift<<2)|step_bits)==127);
 if(!frozen)increment=std::max(1u,increment);
 v.envelope_counter+=increment;
 if(!(v.envelope_counter&0x8000))return;
 v.envelope_counter&=0x7fff;
 v.level=std::clamp(v.level+step,0,32767);
 if(v.phase==Voice::Attack&&v.level==32767){v.phase=Voice::Decay;v.envelope_counter=0;}
 else if(v.phase==Voice::Decay&&v.level<=std::min(32767,int((a&15)+1)*0x800)){v.phase=Voice::Sustain;v.envelope_counter=0;}
 else if(v.phase==Voice::Release&&v.level==0)v.phase=Voice::Off;
 regs[n*8+6]=uint16_t(v.level);
}
void Spu::tick(){if(++sample_clock==768){sample_clock=0;render_sample();}}
void Spu::render_sample(int cd_left,int cd_right){
 const unsigned control=regs[0x1aa/2];int left=0,right=0,send_l=0,send_r=0,previous_output=0;
 noise_clock-=4+((control>>8)&3);
 if(noise_clock<0){unsigned parity=((noise_level>>15)^(noise_level>>12)^(noise_level>>11)^(noise_level>>10)^1)&1;noise_level=uint16_t((noise_level<<1)|parity);const int period=0x20000>>((control>>10)&15);noise_clock+=period;if(noise_clock<0)noise_clock+=period;}
 for(unsigned n=0;n<24;n++){
  auto& v=voices[n];int output=0;const int vl=volume(n*2,regs[n*8]),vr=volume(n*2+1,regs[n*8+1]);
  if((control&0x8000)&&v.phase!=Voice::Off){
   if(!v.decoded)decode(n);
   const unsigned i=v.position>>12,fraction=(v.position>>4)&255;
   int sample=(spu_gauss[255-fraction]*v.samples[i])>>15;
   sample+=(spu_gauss[511-fraction]*v.samples[i+1])>>15;
   sample+=(spu_gauss[256+fraction]*v.samples[i+2])>>15;
   sample=int16_t(sample); // hardware wraps the first three terms before final addition
   sample=clamp16(sample+((spu_gauss[fraction]*v.samples[i+3])>>15));
   if(regs[0x194/2+n/16]&(1u<<(n%16)))sample=int16_t(noise_level);
   envelope(n);output=(sample*v.level)>>15;
   unsigned pitch=regs[n*8+2];
   if(n&&(regs[0x190/2+n/16]&(1u<<(n%16))))pitch=unsigned((int64_t(int16_t(pitch))*(previous_output+32768))>>15)&0xffff;
   v.position+=std::min(pitch,0x4000u);
   while(v.position>=28*4096u){
    v.position-=28*4096u;
    if(v.flags&1){regs[0x19c/2+n/16]|=uint16_t(1u<<(n%16));v.address=(uint32_t(regs[n*8+7])*8)&0x7fff0;if(!(v.flags&2)){v.phase=Voice::Off;v.level=0;regs[n*8+6]=0;}}
    else v.address=(v.address+16)&0x7ffff;
    if(v.phase!=Voice::Off)decode(n);else{v.position=0;break;}
   }
   const int l=(output*vl)>>15;
   const int r=(output*vr)>>15;
   left+=l;right+=r;
   if(regs[0x198/2+n/16]&(1u<<(n%16))){send_l+=l;send_r+=r;}
  }
  previous_output=output;
  if(n==1||n==3){unsigned address=(n==1?0x800:0xc00)+capture_position;if(regs[0x1ac/2]&12)check_irq(address);ram[address/2]=uint16_t(output);}
 }
 // Capture the CD input before SPU volume/enable processing.
 if(regs[0x1ac/2]&12){check_irq(capture_position);check_irq(0x400+capture_position);}
 ram[capture_position/2]=uint16_t(cd_left);ram[(0x400+capture_position)/2]=uint16_t(cd_right);
 capture_position=(capture_position+2)&0x3ff;
 // SPU mute applies to voices, while the independent CD input stays audible.
 if((control&0xc000)!=0xc000){left=right=send_l=send_r=0;}
 if(control&1){
  const int cl=(cd_left*int16_t(regs[0x1b0/2]))>>15,cr=(cd_right*int16_t(regs[0x1b2/2]))>>15;
  left+=cl;right+=cr;if(control&4){send_l+=cl;send_r+=cr;}
 }
 reverb(send_l,send_r);left+=reverb_left;right+=reverb_right;
 const int ml=volume(48,regs[0x180/2]),mr=volume(49,regs[0x182/2]);
 left=int((int64_t(left)*ml)>>15);right=int((int64_t(right)*mr)>>15);
 if(audio.size()>=8820){audio.pop_front();audio.pop_front();dropped_frames++;}
 audio.push_back(int16_t(clamp16(left)));audio.push_back(int16_t(clamp16(right)));generated_frames++;
}
}
