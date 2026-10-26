#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
uint32_t i(unsigned op,unsigned rs,unsigned rt,unsigned imm){return(op<<26)|(rs<<21)|(rt<<16)|(imm&65535);}
uint32_t move(unsigned dst,unsigned src){return(src<<21)|(dst<<11)|0x21;}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);
 auto program=[&](std::initializer_list<uint32_t> words){c.reset(0x1000);unsigned at=0x1000;for(auto word:words){b.write32(at,word);at+=4;}c.r[1]=0x2000;c.r[2]=0x12345678;b.write32(0x2000,0xaaaaaaaa);b.write32(0x2004,0xbbbbbbbb);};
 program({i(35,1,2,0),i(35,1,2,4),move(3,2),move(4,2)});
 c.step();c.step();c.step();c.step();ck(c.r[3]==0x12345678&&c.r[4]==0xbbbbbbbb,"successive loads to same register suppress earlier load");
 program({i(35,1,2,0),i(9,2,2,1),move(3,2)});c.step();c.step();c.step();ck(c.r[3]==0x12345679,"ALU write wins and reads pre-load operand");
 program({i(35,1,2,0),i(35,1,3,4),move(4,2),move(5,3)});for(unsigned n=0;n<4;n++)c.step();ck(c.r[4]==0xaaaaaaaa&&c.r[5]==0xbbbbbbbb,"different load destinations commit independently");
 // MFC0 and MFC2 share the GPR load pipeline with ordinary loads.
 program({i(35,1,2,0),0x40027000,move(3,2),move(4,2)});c.cop0[14]=0x22222222;for(unsigned n=0;n<4;n++)c.step();ck(c.r[3]==0x12345678&&c.r[4]==0x22222222,"MFC0 replaces pending same-register load");
 program({i(35,1,2,0),0x48020000,move(3,2),move(4,2)});c.cop0[12]|=1u<<30;c.gte.write_data(0,0x11223344);for(unsigned n=0;n<4;n++)c.step();ck(c.r[3]==0x12345678&&c.r[4]==0x11223344,"MFC2 replaces pending same-register load");
 // An IRQ between load and consumer must commit that load before handler execution.
 program({i(35,1,2,0),move(3,2)});c.cop0[12]=0x401;c.step();b.irq_mask=b.irq_status=4;c.step();ck(c.pc==0x80000080&&c.r[2]==0xaaaaaaaa&&c.pending_reg==0,"IRQ entry drains pending load");
 b.irq_mask=b.irq_status=0;
 // The entire GPR reaches DMA registers even for SH/SB on their word address.
 for(auto op:{40u,41u}){
  program({i(op,1,2,0)});c.r[1]=0x1f8010b8;c.r[2]=0x11000000;b.dpcr=0;c.step();ck(b.dma[3].control==0x11000000,"SB/SH preserves full DMA control bus value");
 }
 program({i(41,1,2,0)});c.r[1]=0x1f8010f4;c.r[2]=0x008c0000;c.step();ck(b.dicr==0x008c0000,"SH to DICR preserves high enable bits");
 program({i(41,1,2,0)});c.r[1]=0x2100;c.r[2]=0x11223344;b.write32(0x2100,0xaaaaaaaa);c.step();ck(b.read32(0x2100)==0xaaaa3344,"RAM halfword stores stay narrow");
 // A trapping second load does not cancel a completed preceding load.
 program({i(35,1,2,0),i(35,1,2,1)});c.step();c.step();ck(c.r[2]==0xaaaaaaaa&&((c.cop0[13]>>2)&31)==4,"misaligned second load leaves first result intact");
 std::cout<<"PASS: same-register load cancellation, ALU priority, distinct loads, COP0/COP2 pipeline, IRQ drain, full-width DMA stores and alignment exception\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
