#include "ps2_ee.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v,const char* s){if(!v)throw std::runtime_error(s);}
int main(){try{
 ps2::Ee e;e.register_demo();for(unsigned n=0;n<1000;n++)e.step();ck(e.read(0x200,4)==35&&e.r[10].low==0x2e20&&e.gs.regs[14]==0xbb55,"EE program writes GS BGCOLOR, reads PRId and uses MULT/MFLO");
 auto one=[&](uint32_t op){e.pc=0x100000;e.next=e.pc+4;e.delay=false;e.write(e.pc,op,4);e.step();};
 e.r[1].low=uint64_t(-7ll);e.r[2].low=3;one((1u<<21)|(2u<<16)|0x1a);ck(e.lo.low==uint64_t(-2ll)&&e.hi.low==uint64_t(-1ll),"signed division quotient/remainder");
 e.r[1].low=0x80000000;e.r[2].low=0xffffffff;one((1u<<21)|(2u<<16)|0x1a);ck(e.lo.low==0xffffffff80000000ull&&e.hi.low==0,"signed DIV overflow defined");
 e.r[2].low=0;one((1u<<21)|(2u<<16)|0x1b);ck(e.lo.low==~0ull&&e.hi.low==0xffffffff80000000ull,"DIVU zero sign extension");
 e.r[4].high=0x1234;e.hi.low=0xabcdef;one((4u<<11)|0x10);ck(e.r[4].low==0xabcdef&&e.r[4].high==0x1234,"MFHI preserves upper GPR");
 e.r[3].low=0x80000000;one(0x40836000);one(0x40046000);ck(e.r[4].low==0xffffffff80000000ull,"MTC0/MFC0 status latch sign extension");
 e.r[3].low=0;one(0x40837800);ck(e.cop0[15]==0x2e20,"PRId read only");
 e.cop0[12]=2;e.cop0[14]=0x100080;one(0x42000018);ck(e.pc==0x100080&&e.next==0x100084&&!(e.cop0[12]&2),"ERET EXL uses EPC without delay slot");
 e.cop0[12]=6;e.cop0[30]=0x100090;one(0x42000018);ck(e.pc==0x100090&&e.cop0[12]==2,"ERET ERL uses ErrorEPC");
 e.r[2].low=0x8000000000000001ull;one((2u<<16)|(4u<<11)|(4u<<6)|0x3b);ck(e.r[4].low==0xf800000000000000ull,"DSRA sign propagation");
 for(unsigned i=0;i<15;i++){auto a=ps2::Gs::addresses[i];e.write(a,0x1122334455667788ull,8);const auto expected=e.read(a,8);ck(e.read(a+4,4)==uint32_t(expected>>32),"GS upper lane");ck(e.read(a|0xa0000000,8)==expected,"GS uncached alias");}
 e.write(0x12001010,0,8);ck(e.read(0x12001010,8)==0x6000,"IMR reserved bits");e.gs.event(3);ck(e.gs.irq(),"GS local IRQ evaluation");
 e.write(0x12001000,1,4);ck((e.read(0x12001000,8)&31)==2&&e.gs.irq(),"CSR W1C only selected event");e.write(0x12001010,0x200,4);ck(!e.gs.irq(),"IMR mask suppresses local IRQ");
 e.write(0x12001000,0xffffffff,4);ck(e.read(0x12001000,8)==0x4000&&e.read(0x12001010,8)==0x7f00&&e.read(0x120000e0,8)==0,"CSR reset restores latches");
 e.write(0x12001040,~0ull,8);ck(e.read(0x12001040,8)==1,"BUSDIR one-bit latch");e.write(0x12001080,~0ull,8);ck(e.read(0x12001080,8)==0,"SIGLBLID not writable through privileged MMIO");
 bool failed=false;try{e.write(0x12000008,0,8);}catch(const std::runtime_error&){failed=true;}ck(failed,"GS gaps fail explicitly");
 failed=false;try{e.read(0x12000000,2);}catch(const std::runtime_error&){failed=true;}ck(failed,"unsupported GS width explicit");
 std::cout<<"PASS: EE-to-GS program, HI/LO divide corner cases, COP0 moves/PRId/ERET, DSRA, GS lanes/aliases/masks/W1C/reset/local IRQ and explicit gaps\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
