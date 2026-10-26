#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v,const char* msg){if(!v)throw std::runtime_error(msg);}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);b.dpcr=0;b.dicr=0x008a0000;
 // Exact four-instruction continuation in the user's latest trace.
 c.reset(0x80127f40);c.r[3]=0x1f8010f4;c.r[2]=8;
 const uint32_t code[]={0x90640002,0x00021027,0x00822024,0xa0640002};
 for(unsigned i=0;i<4;i++)b.write32(c.pc+4*i,code[i]);
 for(unsigned i=0;i<4;i++)c.step();
 ck(c.r[4]==0x82&&b.dicr==0x00820000,"GTA LBU/NOR/AND/SB continuation updates DICR high byte");
 // W1C must not accidentally acknowledge unselected pending channel flags.
 b.dicr=0x8c8c0000;b.write8(0x1f8010f7,8);
 ck((b.dicr&0x7f000000)==0x04000000&&!(b.dicr&0xffffff),"DICR shifted fullword clears selected flag, replaces controls");
 for(auto alias:{0u,0x80000000u,0xa0000000u}){
  const auto base=0x1f8010b0|alias;
  b.write32(base+4,0x12345678);b.write8(base+5,0xab);ck(b.read32(base+4)==0x1234ab78,"BCR byte enables");
  b.write16(base+6,0xcdef);ck(b.read32(base+4)==0xcdefab78,"BCR upper half preserves lower half");
  c.reset(0x1000);c.r[3]=base+4;c.r[4]=0xdeadbeef;b.write32(0x1000,0xa0640001);c.step();
  ck(b.read32(base+4)==0xcdefef78,"BCR SB ignores upper GPR pins");
  c.reset(0x1000);c.r[3]=base;c.r[4]=0x12345678;b.write32(0x1000,0xa0640001);c.step();
  ck(b.dma[3].address==0x567800,"MADR SB shifts entire GPR then masks address");
  b.write32(base+12,0x200);ck(b.read32(base+8)==0x200,"CHCR mirror write");
  b.write8(base+13,3);ck(b.read32(base+8)==0x300,"CHCR mirror byte write replaces word");
 }
 b.write16(0x1f8010f6,0x0082);ck((b.dicr&0xffffff)==0x820000,"DICR SH lane update");
 // SPU odd SB is ignored; even SB drives low sixteen GPR bits, not low byte.
 c.reset(0x1000);c.r[3]=0x1f801c00;c.r[4]=0x12345678;b.write32(0x1000,0xa0640000);b.write32(0x1004,0xa0640001);c.step();c.step();
 ck(b.read16(0x1f801c00)==0x5678&&b.read8(0x1f801c01)==0x56,"SPU byte access pin width and odd write ignore");
 for(unsigned i=0x260;i<0x280;i+=2){b.write16(0x1f801c00+i,0xffff);ck(b.read16(0x1f801c00+i)==0,"unused SPU halfwords bounded");}
 b.gpu.status=0x89abcdef;ck(b.read16(0x1f801816)==0x89ab,"GPUSTAT upper halfword");
 ck(b.read16(0x1f801826)==uint16_t(b.mdec.status()>>16),"MDEC status upper halfword");
 bool rejected=false;try{b.write32(0x1f801828,0);}catch(const std::runtime_error&){rejected=true;}ck(rejected,"unknown MMIO remains explicit error");
 std::cout<<"PASS: recorded GTA DICR store, DMA masks/W1C/mirrors/aliases, SPU byte pins and added status reads\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
