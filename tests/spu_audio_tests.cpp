#include "core.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>
void ck(bool yes,const char* reason){if(!yes)throw std::runtime_error(reason);}
void block(ps1::Spu& s,unsigned address,unsigned header,unsigned flags,uint8_t data){
 s.ram[address/2]=uint16_t(header|(flags<<8));for(unsigned i=1;i<8;i++)s.ram[address/2+i]=uint16_t(data|(unsigned(data)<<8));
}
void setup(ps1::Spu& s,unsigned voice=0){
 unsigned base=voice*16;s.write(base,0x3fff);s.write(base+2,0x3fff);s.write(base+4,0x1000);s.write(base+6,0x200);
 s.write(base+8,0x000f);s.write(base+10,0x1fc0); // fast attack, full sustain, frozen sustain rate
 s.write(0x180,0x3fff);s.write(0x182,0x3fff);s.write(0x1aa,0xc000);
 s.write(0x188+(voice>=16?2:0),uint16_t(1u<<(voice%16)));
}
int main(){try{
 ps1::Spu s;block(s,0x1000,0,7,0x17);setup(s);s.decode(0);
 ck(s.voices[0].samples[3]==28672&&s.voices[0].samples[4]==4096,"ADPCM low nibble first");
 block(s,0x1000,0x10,7,0x11);s.voices[0].previous=s.voices[0].older=0;s.decode(0);
 ck(s.voices[0].samples[3]==4096&&s.voices[0].samples[4]==7936,"predictor history and rounding");
 block(s,0x1000,0,7,0x88);s.decode(0);ck(s.voices[0].samples[3]==-32768,"signed nibble");
 block(s,0x1000,0xff,7,0x11);s.decode(0);ck(s.voices[0].samples[3]==8,"reserved filter and shift safe fallback");
 block(s,0x1000,0,7,0x77);s.write(0x188,1);s.render_sample();ck(s.voices[0].level>0,"attack begins");
 for(unsigned i=0;i<60;i++)s.render_sample();
 ck(s.voices[0].phase==ps1::Spu::Voice::Sustain&&s.read(12)>0,"attack decay sustain progression");
 ck((s.read(0x19c)&1)&&s.read(14)==0x200&&s.active_voices()==1,"loop ENDX repeat and LSAX");
 ck(std::any_of(s.audio.begin(),s.audio.end(),[](int n){return n!=0;}),"nonzero stereo output");
 s.write(0x188,1);ck(!(s.read(0x19c)&1),"key-on clears ENDX");
 s.write(0x18c,1);for(unsigned i=0;i<8;i++)s.render_sample();ck(s.active_voices()==0&&s.read(12)==0,"key-off release reaches zero");
 block(s,0x1000,0,1,0x77);s.write(0x188,1);for(unsigned i=0;i<29;i++)s.render_sample();ck(s.active_voices()==0&&(s.read(0x19c)&1),"one-shot ends without repeat");
 block(s,0x1000,0,7,0x77);setup(s,23);for(unsigned i=0;i<8;i++)s.render_sample();ck(s.voices[23].level>0,"high key-on bits address voice23");
 s.write(23*16,0);s.write(0x1aa,0x8000);s.audio.clear();s.render_sample();ck(s.audio[0]==0&&s.audio[1]==0,"mute gates output");
 for(unsigned i=0;i<5000;i++)s.render_sample();
 ck(s.audio.size()==8820&&s.dropped_frames>0,"bounded stereo queue");
 ps1::Bus b;for(unsigned i=0;i<767;i++)b.tick_timers();ck(b.spu.generated_frames==0,"sample clock before boundary");b.tick_timers();ck(b.spu.generated_frames==1,"768 clocks per sample");
 b.spu.write(0x1ac,4);b.spu.write(0x1a4,0);b.spu.write(0x1aa,0xc040);for(unsigned i=0;i<768;i++)b.tick_timers();
 ck(b.spu.irq&&(b.irq_status&(1u<<9))&&(b.spu.read(0x1ae)&64),"capture IRQ9 reaches interrupt controller");
 b.write32(0x1f801070,~(1u<<9));b.tick_timers();ck(!(b.irq_status&(1u<<9)),"acknowledge held IRQ does not retrigger");
 b.spu.write(0x1aa,0xc000);b.tick_timers();ck(!b.spu.irq,"SPUCNT disables and acknowledges IRQ");
 b.spu.write(0x1a4,0x200);b.spu.write(0x1a6,0x200);b.spu.write(0x1aa,0xc060);b.spu.dma_write(0);b.tick_timers();ck(b.spu.irq&&(b.irq_status&(1u<<9)),"DMA4 address IRQ");
 b.spu.write(0x1aa,0x20);ck((b.spu.read(0x1ae)&0x180)==0x180,"DMA write request status");
 b.spu.write(0x1aa,0x30);ck((b.spu.read(0x1ae)&0x280)==0x280,"DMA read request status");
 // Direct and sweep volume, readable current volumes.
 ps1::Spu v;ck(v.volume(0,0x3fff)==32766&&v.volume(1,0x4000)==-32768,"signed fixed volume");
 ck(v.volume(0,0xa000)<32766,"volume sweep decreases");
 ck(v.read(0x200)==uint16_t(v.volumes[0].level),"current voice volume read");
 v.volume(48,0x2000);ck(v.read(0x1b8)==0x4000,"current master volume read");
 // Reverb read path remains active when master write enable is clear.
 ps1::Spu r;r.write(0x1a2,0xfffe);r.write(0x1ac,4);r.write(0x184,0x7fff);r.write(0x186,0x7fff);
 r.ram[0x7fff0/2]=10000;r.reverb(0,0);r.reverb(0,0);
 ck(r.reverb_left==9999&&r.reverb_right==9999,"disabled reverb still outputs APF2 delayed memory");
 ck(r.reverb_cursor==0x7fff2,"reverb cursor advances at half rate");
 for(unsigned i=0;i<20;i++)r.reverb(0,0);
 ck(r.reverb_cursor>=0x7fff0&&r.reverb_cursor<=0x7fffe,"reverb small-buffer wrapping");
 // Enabled IIR writes reflected input to the documented SAME address.
 ps1::Spu wet;wet.write(0x1a2,0xf000);wet.write(0x1ac,4);wet.write(0x1aa,0xc080);
 wet.write(0x1c4,0x7fff);wet.write(0x1fc,0x7fff);wet.write(0x1fe,0x7fff);
 wet.write(0x1d4,1);wet.write(0x1d6,2);wet.write(0x1e4,3);wet.write(0x1e6,4);
 wet.reverb(10000,-10000);wet.reverb(10000,-10000);
 ck(int16_t(wet.ram[0x78008/2])>9900&&int16_t(wet.ram[0x78010/2])<-9900,"reverb IIR stereo input");
 ps1::Bus demo;ps1::install_spu_demo(demo);for(unsigned i=0;i<100;i++)demo.spu.render_sample();
 ck(demo.spu.active_voices()==2&&demo.spu.audio[150]!=demo.spu.audio[151],"original stereo test tones");
 std::cout<<"PASS: ADPCM predictors/sign/fallback, ADSR, 24 voices, KON/KOFF/ENDX/loop, stereo/mute, bounded queue, 768-clock sampling, capture/DMA IRQ9 and DMA status\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
