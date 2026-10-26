#include "spu.hpp"
#include <stdexcept>
#include <algorithm>
namespace ps1 {
uint16_t Spu::read(unsigned offset)const{
 if(offset>=0x280||(offset&1))throw std::runtime_error("Invalid SPU halfword read");
 if(offset>=0x260)return 0; // unused SPU register area
 if(offset>=0x200)return uint16_t(volumes[(offset-0x200)/2].level);
 if(offset==0x1b8||offset==0x1ba)return uint16_t(volumes[48+(offset-0x1b8)/2].level);
 if(offset==0x1a8)throw std::runtime_error("SPU FIFO manual read unsupported");
 // Immediate mode/request reflection; hardware transfer latency is approximated.
 if(offset==0x1ae){unsigned mode=(regs[0x1aa/2]>>4)&3;return uint16_t((regs[0x1aa/2]&63)|(irq?64:0)|(mode>=2?128:0)|(mode==2?256:0)|(mode==3?512:0)|(((regs[0x1ac/2]&12)&&(capture_position&0x200))?2048:0));}
 return regs[offset/2];
}
uint32_t Spu::dma_read(){
 if(((regs[0x1aa/2]>>4)&3)!=3)throw std::runtime_error("SPU DMA read requires mode 3");
 check_irq(cursor);uint32_t v=ram[cursor/2];cursor=(cursor+2)&0x7ffff;check_irq(cursor);v|=uint32_t(ram[cursor/2])<<16;cursor=(cursor+2)&0x7ffff;return v;
}
void Spu::dma_write(uint32_t value){
 if(((regs[0x1aa/2]>>4)&3)!=2)throw std::runtime_error("SPU DMA write requires mode 2");
 check_irq(cursor);ram[cursor/2]=uint16_t(value);cursor=(cursor+2)&0x7ffff;check_irq(cursor);ram[cursor/2]=uint16_t(value>>16);cursor=(cursor+2)&0x7ffff;writes+=2;
}
void Spu::flush(){for(uint16_t v:fifo){check_irq(cursor);ram[cursor/2]=v;cursor=(cursor+2)&0x7ffff;}fifo.clear();}
void Spu::write(unsigned offset,uint16_t value){
 if(offset>=0x280||(offset&1))throw std::runtime_error("Invalid SPU halfword write");
 if(offset>=0x200)return;
 if(offset==0x1ae||offset==0x19c||offset==0x19e||offset==0x1b8||offset==0x1ba)return;
 if(offset==0x1a8){
  if(fifo.size()>=32)throw std::runtime_error("SPU FIFO full (32 halfwords)");
  fifo.push_back(value);if(((regs[0x1aa/2]>>4)&3)==1)flush();writes++;return;
 }
 regs[offset/2]=value;writes++;
 if(offset<0x180&&(offset&15)==12)voices[offset/16].level=value&0x7fff;
 if(offset==0x1a2)reverb_cursor=(uint32_t(value)*8)&0x7ffff;
 if(offset==0x1a6)cursor=(uint32_t(value)*8)&0x7ffff;
 if(offset==0x1aa){if(!(value&64))irq=false;if(((value>>4)&3)==1)flush();}
 if(offset>=0x188&&offset<=0x18e){
  const unsigned first=(offset&2)?16:0;
  for(unsigned bit=0;bit<16&&first+bit<24;bit++)if(value&(1u<<bit)){
   const unsigned n=first+bit;auto& v=voices[n];
   if(offset<0x18c){v=Voice{};v.phase=Voice::Attack;v.address=(uint32_t(regs[n*8+3])*8)&0x7fff0;regs[n*8+6]=0;regs[0x19c/2+n/16]&=uint16_t(~(1u<<(n%16)));}
   else if(v.phase!=Voice::Off){v.phase=Voice::Release;v.envelope_counter=0;}
  }
 }
}
}
