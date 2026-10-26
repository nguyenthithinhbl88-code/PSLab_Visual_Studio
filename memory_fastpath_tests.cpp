#include "core.hpp"
#include <iostream>
#include <stdexcept>
#include <memory>
namespace {void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}}
int main(){try{
 auto b=std::make_unique<ps1::Bus>();uint32_t random=0x12345678;
 for(unsigned i=0;i<200000;i++){
  random=random*1664525+1013904223;uint32_t physical=random&0x7fffff;uint32_t alias=i%3==0?0:i%3==1?0x80000000:0xa0000000;uint32_t a=physical|alias;uint8_t value=uint8_t(random>>24);
  b->write8(a,value);check(b->ram[physical&0x1fffff]==value&&b->read8(a)==value,"byte RAM mirror endian/alias");
  uint16_t word=uint16_t(random>>8);a&=~1u;b->write16(a,word);unsigned index=(physical&0x1fffff)&~1u;check(b->ram[index]==uint8_t(word)&&b->ram[index+1]==uint8_t(word>>8)&&b->read16(a)==word,"halfword RAM mirror endian/alias");
 }
 for(auto alias:{0u,0x80000000u,0xa0000000u}){
  b->write8(alias|0x1f8003ff,0xa5);check(b->read8(alias|0x1f8003ff)==0xa5,"scratch final byte");b->write16(alias|0x1f8003fe,0x1234);check(b->read16(alias|0x1f8003fe)==0x1234,"scratch final halfword");
  b->bios[0x7fffe]=0x78;b->bios[0x7ffff]=0x56;check(b->read16(alias|0x1fc7fffe)==0x5678&&b->read8(alias|0x1fc7ffff)==0x56,"BIOS final byte/halfword");
 }
 bool error=false;try{b->write16(1,0);}catch(const std::runtime_error&){error=true;}check(error,"unaligned halfword store rejected");error=false;try{b->read16(1);}catch(const std::runtime_error&){error=true;}check(error,"unaligned halfword load rejected");
 error=false;try{b->write8(0xbfc00000,7);}catch(const std::runtime_error&){error=true;}check(error,"BIOS remains read only");
 b->write32(0xfffe0130,0x12345678);check(b->read16(0xfffe0130)==0x5678&&b->read8(0xfffe0133)==0x12,"cache-control MMIO unaffected");
 b->write16(0x1f801074,0x204);check(b->irq_mask==0x204,"IRQ MMIO unaffected");
 std::cout<<"PASS: 200000 byte/halfword RAM alias checks, scratch/ROM boundaries, alignment, write protection and MMIO fallbacks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
