#include "ps2_ee.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
namespace ps2 {
namespace {
uint32_t* dma_register(Ee::Dma& d,unsigned offset,unsigned channel){
 switch(offset){case 0:return &d.chcr;case 0x10:return &d.madr;case 0x20:return &d.qwc;case 0x30:return &d.tadr;case 0x40:if(channel<3)return &d.asr0;break;case 0x50:if(channel<3)return &d.asr1;break;case 0x80:if(channel>=8)return &d.sadr;break;}return nullptr;
}
std::runtime_error unsupported(const char* name,uint32_t a){std::ostringstream o;o<<name<<" @ 0x"<<std::hex<<a;return std::runtime_error(o.str());}
}
void Ee::reset_hardware(){timers={};dma={};dmac={};sif={};intc_stat=intc_mask=gif_ctrl=gif_mode=0;dma_enable=0x1201;bus_phase=video_dot=video_line=0;video_frames=hardware_ticks=dma_quadwords=interrupts=0;hblank=vblank=previous_gs_irq=false;}
bool Ee::mmio_read(uint32_t a,uint32_t& v)const{
 if(a>=0x10000000&&a<0x10002000){unsigned n=(a-0x10000000)/0x800,off=(a-0x10000000)%0x800;if(n<4){const auto& t=timers[n];switch(off){case 0:v=t.count;return true;case 0x10:v=t.mode;return true;case 0x20:v=t.compare;return true;case 0x30:if(n<2){v=t.hold;return true;}break;}}return false;}
 for(unsigned n=0;n<dma.size();n++)if(a>=dma_bases[n]&&a<dma_bases[n]+0x100){auto copy=dma[n];if(auto reg=dma_register(copy,a-dma_bases[n],n)){v=*reg;return true;}return false;}
 if(a>=0x1000e000&&a<=0x1000e060&&!(a&15)){v=dmac[(a-0x1000e000)/16];return true;}
 switch(a){
 case 0x1000f000:v=intc_stat;return true;case 0x1000f010:v=intc_mask;return true;
 case 0x1000f520:v=dma_enable;return true;case 0x1000f590:v=dma_enable;return true;
 case 0x1000f200:v=sif[0];return true;case 0x1000f210:v=sif[1];return true;case 0x1000f220:v=sif[2];return true;case 0x1000f230:v=sif[3];return true;case 0x1000f240:v=sif[4]|0xf0000102;return true;case 0x1000f260:v=sif[5];return true;
 case 0x10003000:v=gif_ctrl;return true;case 0x10003010:v=gif_mode;return true;
 case 0x10003020:v=(gif_mode&5)|(gif_ctrl&8)|((dma[2].chcr&0x100)?0xe00:0);return true;
 case 0x10003040:v=uint32_t(gs.gif_tag_low);return true;case 0x10003050:v=uint32_t(gs.gif_tag_low>>32);return true;case 0x10003060:v=uint32_t(gs.gif_tag_high);return true;case 0x10003070:v=uint32_t(gs.gif_tag_high>>32);return true;
 case 0x10003080:{unsigned nr=gs.gif_format>=2?1:gs.gif_nreg;v=nr?((gs.gif_remaining+nr-1)/nr)&0x7fff:0;if(gs.gif_remaining&&nr)v|=((gs.gif_cursor%nr+1)&15)<<16;return true;}
 case 0x10003090:{unsigned nr=gs.gif_format>=2?1:gs.gif_nreg;v=nr?((gs.gif_remaining+nr-1)/nr)&0x7fff:0;return true;}case 0x100030a0:v=uint32_t(gs.gif_tag_low);return true;
 default:return false;
 }
}
bool Ee::mmio_write(uint32_t a,uint32_t v){
 if(a>=0x10000000&&a<0x10002000){unsigned n=(a-0x10000000)/0x800,off=(a-0x10000000)%0x800;if(n<4){auto& t=timers[n];switch(off){case 0:t.count=uint16_t(v);t.divider=0;return true;case 0x10:t.mode=uint16_t((t.mode&0xc00&~v)|(v&0x3ff));t.divider=0;return true;case 0x20:t.compare=uint16_t(v);return true;case 0x30:if(n<2){t.hold=uint16_t(v);return true;}break;}}return false;}
 for(unsigned n=0;n<dma.size();n++)if(a>=dma_bases[n]&&a<dma_bases[n]+0x100){
  auto& d=dma[n];unsigned off=a-dma_bases[n];auto reg=dma_register(d,off,n);if(!reg)return false;
  if(off==0&&(v&0x100)){
   if(n!=2||(v&12))throw unsupported("Only normal-mode GIF DMA implemented",a);
   if(d.madr&0x80000000u)throw unsupported("GIF scratchpad DMA not implemented",d.madr);
   if(dmac[0]&0x3fe)throw unsupported("DMAC cycle stealing/MFIFO/stall control not implemented",0x1000e000);
  }
  if(off!=0&&(d.chcr&0x100))throw unsupported("DMA address/count write while running unsupported",a);
  if(off==0x10||off==0x30||off==0x40||off==0x50)v&=0xfffffff0;
  if(off==0x20)v&=0xffff;
  if(off==0x80)v&=0x3ff0;
  *reg=v;return true;
 }
 if(a>=0x1000e000&&a<=0x1000e060&&!(a&15)){unsigned i=(a-0x1000e000)/16;if(i==1){dmac[1]&=~(v&0xe3ff);dmac[1]^=v&0x63ff0000;}else dmac[i]=v;return true;}
 switch(a){
 case 0x1000f000:intc_stat&=~(v&0x7fff);return true;case 0x1000f010:intc_mask^=v&0x7fff;return true;
 case 0x1000f520:return true; // Read-only status alias.
 case 0x1000f590:dma_enable=v;return true;
 case 0x1000f200:sif[0]=v;return true;case 0x1000f210:return true; // SMCOM is owned by IOP.
 case 0x1000f220:sif[2]|=v;return true;case 0x1000f230:sif[3]&=~v;return true;
 case 0x1000f240:sif[4]=(sif[4]&~0x100u)|(v&0x100);return true;case 0x1000f260:sif[5]=v;return true;
 case 0x10003000:
  if(v&1){gs.gif_remaining=gs.gif_cursor=0;gs.gif_tag_low=gs.gif_tag_high=0;gs.gif_faulted=false;gs.gif_format=gs.gif_nreg=0;}
  gif_ctrl=v&8;return true;
 case 0x10003010:gif_mode=v&5;return true;
 case 0x10003020:case 0x10003040:case 0x10003050:case 0x10003060:case 0x10003070:case 0x10003080:case 0x10003090:case 0x100030a0:return true; // Read-only diagnostics.
 default:return false;
 }
}
void Ee::update_interrupts(){
 bool line=gs.irq();if(line&&!previous_gs_irq)intc_stat|=1;previous_gs_irq=line;
 cop0[13]=(cop0[13]&~0xc00u)|((intc_stat&intc_mask)?0x400u:0)|(((dmac[1]&(dmac[1]>>16)&0x63ff)||(dmac[1]&0x8000))?0x800u:0);
}
bool Ee::take_interrupt(){
 if((cop0[12]&0x10007)!=0x10001||!(cop0[12]&cop0[13]&0x8c00))return false;
 cop0[14]=delay?pc-4:pc;cop0[13]=(cop0[13]&~0x8000007cu)|(delay?0x80000000u:0);cop0[12]|=2;
 pc=(cop0[12]&(1u<<22))?0xbfc00400:0x80000200;next=pc+4;delay=false;interrupts++;return true;
}
void Ee::hardware_tick(){
 if(random_reloaded)random_reloaded=false;else cop0[1]=(cop0[1]<=cop0[6]||cop0[1]>47)?47:cop0[1]-1;
 hardware_ticks++;if(cop0[9]==cop0[11])cop0[13]|=0x8000;
 if(++bus_phase<2){update_interrupts();return;}bus_phase=0;
 bool old_h=hblank,old_v=vblank;const bool pal=((gs.regs[1]>>13)&3)==3;
 unsigned line_clocks=pal?9436:9370,lines=pal?312:262,active_lines=pal?286:240;
 if(++video_dot>=line_clocks){video_dot=0;if(++video_line>=lines){video_line=0;video_frames++;gs.regs[15]^=0x2000;}}
 hblank=video_dot>=line_clocks*4/5;vblank=video_line>=active_lines;
 bool rise_h=!old_h&&hblank;
 if(rise_h)gs.event(4);
 if(!old_v&&vblank){intc_stat|=4;gs.event(8);}if(old_v&&!vblank)intc_stat|=8;
 for(unsigned i=0;i<4;i++){
  auto& t=timers[i];if(!(t.mode&0x80))continue;
  const bool gate=(t.mode&8)?vblank:hblank,old_gate=(t.mode&8)?old_v:old_h;
  // Functional gate approximation: blank timing is deliberately approximate.
  if((t.mode&4)&&!((t.mode&3)==3&&!(t.mode&8))){unsigned gm=(t.mode>>4)&3;if(gm==0&&gate)continue;if((gm==1&&!old_gate&&gate)||(gm==2&&old_gate&&!gate)||(gm==3&&old_gate!=gate)){t.count=0;t.divider=0;}}
  unsigned clock=t.mode&3;if(clock==3){if(!rise_h)continue;}else{unsigned div=clock==0?1:clock==1?16:256;if(++t.divider<div)continue;t.divider=0;}
  uint16_t previous=t.count;t.count++;
  if(t.count==t.compare){if((t.mode&0x100)&&!(t.mode&0x400)){t.mode|=0x400;intc_stat|=1u<<(9+i);}if(t.mode&0x40)t.count=0;}
  if(previous==0xffff&&(t.mode&0x200)&&!(t.mode&0x800)){t.mode|=0x800;intc_stat|=1u<<(9+i);}
 }
 auto& d=dma[2];
 if((d.chcr&0x100)&&(dmac[0]&1)&&!(dma_enable&0x10000)&&!(gif_ctrl&8)&&!(gif_mode&1)){
  if(dmac[0]&0x3fe)throw unsupported("Active DMA uses unsupported D_CTRL mode",0x1000e000);
  if((dmac[2]&0x80000000u)&&!(dmac[2]&(1u<<18))){update_interrupts();return;}
  if(d.qwc){
   // Only main RAM is accepted. Invalid transfers set BUSERR and preserve progress at the failing QW.
   if(uint64_t(d.madr)+16>ram.size()){d.chcr&=~0x100u;dmac[1]|=0x8000;throw unsupported("GIF DMA source outside main RAM",d.madr);}
   uint64_t low=read(d.madr,8),high=read(d.madr+8,8);gs.gif_push(low,high);d.madr+=16;d.qwc--;dma_quadwords++;
  }
  if(!d.qwc){d.chcr&=~0x100u;dmac[1]|=4;}
 }
 update_interrupts();
}
std::string Ee::hardware_diagnostic()const{
 std::ostringstream o;o<<"EE hardware: approximate 1 instruction/EE tick, 2 EE ticks/bus tick; video/gates not cycle accurate.\r\n";
 o<<"INTC_STAT="<<std::hex<<intc_stat<<" MASK="<<intc_mask<<" D_ENABLE="<<dma_enable<<" interrupt entries="<<std::dec<<interrupts<<"\r\n";
 o<<"Bus model video="<<video_line<<":"<<video_dot<<" frames="<<video_frames<<" DMA QW="<<dma_quadwords<<"\r\n";
 for(unsigned i=0;i<4;i++)o<<"T"<<i<<" COUNT="<<std::hex<<timers[i].count<<" MODE="<<timers[i].mode<<" COMP="<<timers[i].compare<<" HOLD="<<timers[i].hold<<"\r\n";
 for(unsigned i=0;i<dmac.size();i++)o<<"DMAC global["<<i<<"]="<<std::hex<<dmac[i]<<"\r\n";
 for(unsigned i=0;i<dma.size();i++)o<<"DMA"<<std::dec<<i<<" CHCR="<<std::hex<<dma[i].chcr<<" MADR="<<dma[i].madr<<" QWC="<<dma[i].qwc<<" TADR="<<dma[i].tadr<<" ASR="<<dma[i].asr0<<":"<<dma[i].asr1<<" SADR="<<dma[i].sadr<<"\r\n";
 o<<"SIF (IOP not running; no fabricated reply): ";for(auto value:sif)o<<std::hex<<value<<" ";o<<"\r\nGIF CTRL="<<gif_ctrl<<" MODE="<<gif_mode<<"\r\n";return o.str();
}
void Ee::dma_demo(){
 gpu_demo(); // Reuse authored packet data, but send it via DMA instead of CPU SQ.
 const uint32_t code[]={0x08040000,0};for(unsigned i=0;i<2;i++)write(0x100000+i*4,code[i],4);
 write(0x1000a010,0x100,4);write(0x1000a020,1+(read(0x100,8)&0x7fff),4);write(0x1000e000,1,4);write(0x1000a000,0x101,4);
}
}
