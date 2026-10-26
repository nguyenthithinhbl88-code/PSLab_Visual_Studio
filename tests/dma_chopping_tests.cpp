#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);auto& d=b.dma[3];
 auto fifo=[&](unsigned words,unsigned first){for(unsigned n=0;n<words;n++){uint32_t v=first+n;for(unsigned j=0;j<4;j++)b.cd.data.push_back(uint8_t(v>>(j*8)));}};
 b.cd.requested=true;b.cd.last_lba=3869;b.dpcr=0x3333bbbb;b.dicr=0x00820000;
 fifo(504,0x12340008);b.write32(0x1f8010b0,0x8014cba8);b.write32(0x1f8010b4,0x1f8);
 // Exact instruction and register state in the supplied v21 trace.
 c.reset(0x80127fe4);c.r[5]=0x1f8010b8;c.r[21]=0x11400100;b.write32(c.pc,0xacb50000);c.step();
 ck(d.remaining==503&&d.address==0x14cbac&&b.read32(0x14cba8)==0x12340008,"GTA SW starts first one-word chopped burst");
 ck((d.control&0x11000000)==0x01000000&&d.chop_wait==15,"busy remains, trigger clears; CPU step advances one of sixteen wait ticks");
 const auto before=b.dma_words;
 b.write32(0x1f8010f0,b.dpcr);ck(b.dma_words==before&&d.chop_wait==15,"MMIO re-entry cannot consume CPU wait interval");
 for(unsigned i=0;i<14;i++){b.tick_timers();}
 ck(b.dma_words==before,"no transfer before CPU window elapses");
 b.tick_timers();ck(b.dma_words==before+1&&d.chop_wait==16,"next one-word burst after exact CPU window");
 while(d.control&(1u<<24))b.tick_timers();
 ck(d.chop_bursts==504&&d.remaining==0&&(d.block&65535)==0&&d.address==0x14d388,"all 504 words, BCR and MADR updated");
 for(unsigned n=0;n<504;n++)ck(b.read32(0x14cba8+n*4)==0x12340008+n,"no skipped/repeated data across GTA burst windows");
 ck(b.cd.data.empty()&&!(b.irq_status&8),"real trace DICR mask does not enable DMA3 interrupt");
 // Short device FIFO, wait/re-entry, DPCR suspend and final completion IRQ.
 b.dicr=0x00880000;b.irq_status=0;fifo(2,0xabcd0000);
 b.write32(0x1f8010b0,0x2000);b.write32(0x1f8010b4,0x10006);b.write32(0x1f8010b8,0x11120100); // 4-word DMA / 2-tick CPU window
 ck(d.remaining==4&&d.chop_wait==2&&!(b.dicr&0x08000000),"partial FIFO is not false completion");
 b.tick_timers();ck(d.remaining==4,"partial burst CPU wait");b.tick_timers();ck(d.remaining==4&&d.chop_wait==0,"empty FIFO waits without data loss");
 b.write32(0x1f8010f0,0);fifo(4,0xabcd0002);for(unsigned i=0;i<4;i++)b.tick_timers();ck(d.remaining==4,"disabled channel cannot progress");
 b.write32(0x1f8010f0,0x8000);ck(!d.active&&!(d.control&0x11000000)&&d.block==0x10000&&(b.dicr&0x88000000u)==0x88000000u&&(b.irq_status&8),"final slice only raises completion IRQ and preserves BCR high half");
 for(unsigned n=0;n<6;n++)ck(b.read32(0x2000+n*4)==0xabcd0000+n,"partial/resumed FIFO integrity");
 const auto words=b.dma_words;for(unsigned i=0;i<100;i++)b.tick_timers();ck(b.dma_words==words,"completed channel never repeats");
 // Zero count means 65536; explicit CHCR stop cancels pending burst state.
 fifo(1,0xfeed0000);b.write32(0x1f8010b4,0);b.write32(0x1f8010b8,0x11000100);
 ck(d.remaining==65535,"zero BCR means 65536 words");b.write32(0x1f8010b8,0);ck(!d.active&&!d.remaining&&!d.chop_wait,"stop cancels scheduling");
 // GPU/SPU manual chopping share the scheduler, retaining their device semantics.
 b.write32(0x1f8010f0,0x88800);for(unsigned i=0;i<6;i++)b.write32(0x3000+i*4,0);
 const auto gpu_words=b.gpu.words;b.write32(0x1f8010a0,0x3000);b.write32(0x1f8010a4,6);b.write32(0x1f8010a8,0x11210101);
 ck(b.gpu.words==gpu_words+2,"GPU two-word burst");for(unsigned i=0;i<3;i++)b.tick_timers();ck(b.gpu.words==gpu_words+2,"GPU CPU wait");b.tick_timers();ck(b.gpu.words==gpu_words+4,"GPU second burst");for(unsigned i=0;i<4;i++)b.tick_timers();ck(b.gpu.words==gpu_words+6&&!b.dma[2].active,"GPU completion");
 b.spu.write(0x1aa,0x20);b.spu.write(0x1a6,0x100);b.write32(0x4000,0xdeadbeef);b.write32(0x4004,0x01234567);
 b.write32(0x1f8010c0,0x4000);b.write32(0x1f8010c4,2);b.write32(0x1f8010c8,0x11000101);b.tick_timers();
 ck(b.spu.ram[0x800/2]==0xbeef&&b.spu.ram[0x804/2]==0x4567&&!b.dma[4].active,"SPU chopped transfer maintains word ordering");
 std::cout<<"PASS: exact GTA chopped DMA, CPU windows, data/FIFO continuity, BCR/MADR, trigger/busy, DPCR suspension, IRQ gating/completion, cancellation and GPU/SPU bursts\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
