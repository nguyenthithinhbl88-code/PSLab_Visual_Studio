#include "core.hpp"
#include <cmath>
namespace ps1 {
void install_spu_demo(Bus& bus){
 bus.spu=Spu{};auto& s=bus.spu;
 // Original 280-sample sine encoded as filter-0 ADPCM. This is not Sony audio.
 for(unsigned block=0;block<10;block++){
  const unsigned address=0x1000+block*16;s.ram[address/2]=uint16_t(1|((block==9?3:block==0?4:0)<<8));
  for(unsigned i=0;i<28;i++){
   int nibble=int(std::lround(7*std::sin((block*28+i)*6.283185307179586/280.0)))&15;
   unsigned byte=address+2+i/2,shift=(byte&1)*8+(i&1)*4;
   s.ram[byte/2]=uint16_t((s.ram[byte/2]&~(15u<<shift))|(unsigned(nibble)<<shift));
  }
 }
 s.write(0x180,0x3000);s.write(0x182,0x3000);s.write(0x1ac,4);s.write(0x1aa,0xc000);
 for(unsigned voice=0;voice<2;voice++){
  unsigned r=voice*16;s.write(r,voice?0:0x2800);s.write(r+2,voice?0x2800:0);
  s.write(r+4,voice?0x3000:0x2800);s.write(r+6,0x200);s.write(r+8,0x000f);s.write(r+10,0x1fc6);
 }
 s.write(0x188,3);bus.write32(0,0x08000000);bus.write32(4,0);
}
}
