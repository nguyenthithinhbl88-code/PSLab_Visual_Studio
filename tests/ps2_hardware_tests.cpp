#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
template<class F> void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}check(caught,"unsupported hardware request must fail");}
}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();e->demo();
 check(e->read(0xb000f520,4)==0x1201,"DMAC enable boot value");e->write(0xb000f590,0x10000,4);check(e->read(0x1000f520,4)==0x10000,"enable write/read aliases");
 e->write(0x1000f010,0x201,4);e->write(0x1000f010,1,4);check(e->intc_mask==0x200,"INTC mask toggles");e->intc_stat=0x205;e->write(0x1000f000,4,4);check(e->intc_stat==0x201,"INTC W1C preserves other sources");
 e->write(0x1000e010,0x40000,4);check(e->dmac[1]==0x40000,"D_STAT mask toggles");e->dmac[1]|=4;e->update_interrupts();check(e->cop0[13]&0x800,"DMAC pending Cause");e->write(0x1000e010,4,4);check(e->dmac[1]==0x40000,"D_STAT channel W1C");e->write(0x1000e010,0x40000,4);check(e->dmac[1]==0,"D_STAT toggle off");
 for(unsigned i=0;i<10;i++){
  uint32_t base=e->dma_bases[i];e->write(base+0x10,0x12345,4);e->write(base+0x20,0xffff1234,4);e->write(base+0x30,0x76543,4);
  check(e->read(base+0x10,4)==0x12340&&e->read(base+0x20,4)==0x1234&&e->read(base+0x30,4)==0x76540,"DMA bank masks/latches");
  if(i!=2)fails([&]{e->write(base,0x100,4);});
 }
 fails([&]{e->write(0x1000a000,0x105,4);});check(!(e->dma[2].chcr&0x100),"unsupported chain start is atomic");
 e->reset();e->write(0x1000f200,0x1234,4);e->write(0x1000f210,0xffff,4);e->write(0x1000f220,0x10000,4);e->write(0x1000f220,0x20000,4);e->sif[3]=0xf0000;e->write(0x1000f230,0x40000,4);
 check(e->sif[0]==0x1234&&e->sif[1]==0&&e->sif[2]==0x30000&&e->sif[3]==0xb0000,"SIF ownership/set/clear without false IOP reply");
 // Four timers: /1, /16, /256 and external HBlank. MODE flags are W1C and IRQs are edge latched.
 e->reset();e->write(0x10000020,2,4);e->write(0x10000010,0x1c0,4);for(unsigned i=0;i<4;i++)e->hardware_tick();check(e->timers[0].count==0&&(e->timers[0].mode&0x400)&&(e->intc_stat&0x200),"timer compare/reset/IRQ");
 e->write(0x1000f000,0x200,4);for(unsigned i=0;i<4;i++)e->hardware_tick();check(!(e->intc_stat&0x200),"flag must be cleared before another timer edge");
 e->write(0x10000010,0x5c0,4);for(unsigned i=0;i<4;i++)e->hardware_tick();check(e->intc_stat&0x200,"MODE W1C rearms timer");
 e->reset();e->write(0x10000810,0x81,4);e->write(0x10001010,0x82,4);for(unsigned i=0;i<512;i++)e->hardware_tick();check(e->timers[1].count==16&&e->timers[2].count==1,"timer clock dividers");
 e->write(0x10001800,0xffff,4);e->write(0x10001810,0x280,4);e->hardware_tick();e->hardware_tick();check((e->timers[3].mode&0x800)&&(e->intc_stat&(1u<<12)),"timer overflow");
 e->reset();e->write(0x10000010,0x83,4);e->video_dot=7495;e->hardware_tick();e->hardware_tick();check(e->timers[0].count==1,"HBlank source reaches timer");
 e->video_line=239;e->video_dot=9369;e->hardware_tick();e->hardware_tick();check(e->vblank&&(e->intc_stat&4)&&(e->gs.regs[15]&8),"VBlank start events");
 e->video_line=261;e->video_dot=9369;e->hardware_tick();e->hardware_tick();check(!e->vblank&&(e->intc_stat&8)&&e->video_frames==1,"VBlank end/frame");
 // Normal GIF DMA can pause/restart; completion is raised only after the actual data reaches GS.
 e->dma_demo();e->write(0x1000f590,0x10000,4);for(unsigned i=0;i<8;i++)e->step();check(e->dma_quadwords==0&&e->dma[2].qwc==30,"DMA disabled suspends progress");e->write(0x1000f590,0,4);e->write(0x10003000,8,4);for(unsigned i=0;i<8;i++)e->step();check(e->dma_quadwords==0,"GIF temporary stop suspends DMA");e->write(0x10003000,0,4);
 for(unsigned i=0;i<100;i++){e->step();}check(e->dma_quadwords==30&&e->dma[2].qwc==0&&!(e->dma[2].chcr&0x100)&&(e->dmac[1]&4)&&e->gs.primitives==6,"DMA RAM -> GIF -> GS completes six primitives");
 e->reset();e->write(0x1000a010,0x1fffff0,4);e->write(0x1000a020,2,4);e->write(0x1fffff0,0,8);e->write(0x1fffff8,0,8);e->write(0x1000e000,1,4);e->write(0x1000a000,0x100,4);e->hardware_tick();e->hardware_tick();e->hardware_tick();fails([&]{e->hardware_tick();});check(e->dma[2].qwc==1&&(e->dmac[1]&0x8000),"DMA bus error preserves failed QW progress");
 // INTC -> EE exception, BD/EPC and ERET. IE+EIE+mask required; no IRQ counted as an instruction.
 e->demo();e->intc_stat=e->intc_mask=0x200;e->cop0[12]=0x10401;uint32_t resume=e->pc;e->step();check(e->pc==0x80000200&&e->cop0[14]==resume&&(e->cop0[12]&2)&&e->instructions==0&&e->interrupts==1,"EE INT0 entry");
 e->write(0x80000200,0x42000018,4);e->intc_stat=0;e->step();check(e->pc==resume&&!(e->cop0[12]&2),"ERET from hardware IRQ");
 e->demo();e->write(e->pc,0x10000002,4);e->cop0[12]=0x410401;e->step();e->intc_stat=e->intc_mask=1;e->step();check(e->pc==0xbfc00400&&e->cop0[14]==0x100000&&(e->cop0[13]&0x80000000),"bootstrap vector and BD/EPC");
 e->demo();e->cop0[9]=10;e->cop0[11]=11;e->step();check(e->cop0[13]&0x8000,"Count Compare pending");e->r[2].low=100;e->write(e->pc,0x40825800,4);e->step();check(!(e->cop0[13]&0x8000),"MTC0 Compare acknowledges timer");
 e->write(0x20000100,0xabcdef01,4);check(e->read(0x80000100,4)==0xabcdef01,"uncached RAM alias");e->write(0x30100000,0x12345678,4);check(e->read(0x100000,4)==0x12345678,"accelerated RAM alias");
 fails([&]{e->read(0x10000004,4);});fails([&]{e->read(0x10001830,4);});fails([&]{e->read(0x10002000,4);});
 std::cout<<"PASS: EE MMIO banks, timers/HBlank/VBlank, INTC/DMAC/SIF, real GIF DMA pause/completion/bus error, IRQ/BD/ERET/Compare and RAM aliases\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}}
