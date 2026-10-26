#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);b.dpcr=0;b.write32(0x1f8010f4,0x008a0000);
 // Exact faulting opcode/address/PC and DICR value from the v20.0 real log.
 c.reset(0x80127f40);c.r[3]=0x1f8010f4;b.write32(c.pc,0x90640002);b.write32(c.pc+4,0);c.step();
 ck(c.r[4]==0,"LBU retains its load delay");c.step();ck(c.r[4]==0x8a,"recorded LBU reads DICR byte 2");
 auto lanes=[&](uint32_t base){
  const uint32_t word=b.read32(base);const auto before=b.dma_words;const auto irq=b.irq_status;
  for(auto alias:{0u,0x80000000u,0xa0000000u}){
   for(unsigned n=0;n<4;n++)ck(b.read8((base|alias)+n)==uint8_t(word>>(8*n)),"DMA byte lane/alias matches register value");
   for(unsigned n=0;n<4;n+=2)ck(b.read16((base|alias)+n)==uint16_t(word>>(8*n)),"DMA halfword lane/alias matches register value");
  }
  ck(b.dma_words==before&&b.irq_status==irq&&b.read32(base)==word,"subword reads have no transfer or ACK side effects");
 };
 for(unsigned ch=0;ch<7;ch++){
  const auto base=0x1f801080+ch*16;b.write32(base,0x00c3b2a1);b.write32(base+4,0x89abcdef);b.write32(base+8,0x55ccaa33);
  lanes(base);lanes(base+4);lanes(base+8);
 }
 b.dpcr=0x76543210;lanes(0x1f8010f0);b.dpcr=0;
 b.dicr=0xd7abcdef;lanes(0x1f8010f4);b.dicr=0x008a0000;
 // Signed/unsigned loads from high lanes with the real MIPS load pipeline.
 b.write32(0x1f8010f4,0x008a0000);
 for(auto op:{32u,36u,33u,37u}){
  c.reset(0x1000);c.r[3]=0x1f8010f4;b.write32(0x1000,(op<<26)|(3<<21)|(4<<16)|2);b.write32(0x1004,0);c.step();c.step();
  ck(c.r[4]==(op==32?0xffffff8au:0x8au),"LB/LBU/LH/LHU select and extend DICR lane");
 }
 bool rejected=false;try{b.read16(0x1f8010f5);}catch(const std::runtime_error&){rejected=true;}ck(rejected,"unaligned halfword remains an error");
 ck(b.read32(0x1f8010bc)==b.read32(0x1f8010b8),"documented CHCR mirror");
 std::cout<<"PASS: recorded GTA LBU opcode, all DMA0..6 register byte/halfword lanes, DPCR/DICR, aliases, load sign/delay and no read side effects\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
