#include "core.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool value,const char* text){if(!value)throw std::runtime_error(text);}
uint32_t instruction(unsigned op,unsigned rs,unsigned rt,unsigned imm=0){return (op<<26)|(rs<<21)|(rt<<16)|imm;}
int main(){try{
 auto bp=std::make_unique<ps1::Bus>();auto& b=*bp;ps1::Cpu c(b);
 auto setup=[&](uint32_t ins){c.reset(0x1000);c.cop0[12]=2;b.write32(0x1000,ins);c.r[1]=0x80002000;c.r[2]=0x12345678;b.write32(0x2000,0xaabbccdd);};
 for(unsigned op:{32u,33u,34u,35u,36u,37u,38u,40u,41u,42u,43u,46u,50u,58u}){
  setup(instruction(op,1,2));c.cop0[12]|=1u<<30;c.step();
  const unsigned expected=(op>=40&&op!=50)?5:4;
  ck(((c.cop0[13]>>2)&31)==expected&&c.cop0[8]==0x80002000,"user data access exception and BadVAddr");
  ck(b.read32(0x2000)==0xaabbccdd&&c.r[2]==0x12345678,"faulting access has no memory/GPR side effect");
 }
 setup(instruction(40,1,2));c.cop0[12]|=0x10000;c.step();ck(((c.cop0[13]>>2)&31)==5,"cache isolation cannot bypass user access check");
 setup(0);c.pc=0x80001000;c.next_pc=c.pc+4;c.pending_reg=2;c.pending_value=99;c.step();
 ck(c.cop0[8]==0x80001000&&c.cop0[14]==0x80001000&&c.r[2]==99&&c.instructions==0,"user fetch fault drains older load before fetching");
 setup(instruction(35,1,2));c.r[1]=0x2000;c.step();b.write32(0x1004,0);c.step();ck(c.r[2]==0xaabbccdd,"KUSEG user read works with load delay");
 setup(0x40026000);c.step();ck(((c.cop0[13]>>2)&31)==11&&!(c.cop0[13]&0x30000000),"user COP0 without CU0 traps CE0");
 setup(0x42000010);c.step();ck(((c.cop0[13]>>2)&31)==11,"user RFE without CU0 traps");
 setup(0x40026000);c.cop0[12]|=1u<<28;c.step();b.write32(0x1004,0);c.step();ck(c.r[2]==0x10000002,"CU0 allows user MFC0");
 for(unsigned reg:{0u,1u,2u,4u,10u}){setup(0x40020000|(reg<<11));c.cop0[12]=0;c.step();ck(((c.cop0[13]>>2)&31)==10,"missing PS1 COP0 registers raise RI");}
 setup(0x40028000);c.step();ck(!c.first_fault.valid,"garbage COP0 read keeps documented user-mode exception bypass");
 for(unsigned op:{48u,56u}){setup(instruction(op,1,2));c.step();ck(((c.cop0[13]>>2)&31)==11,"LWC0/SWC0 are CpU rather than RI");}
 setup(instruction(50,1,2));c.step();ck(((c.cop0[13]>>2)&31)==11&&((c.cop0[13]>>28)&3)==2,"disabled COP2 has priority over its bad address");
 setup(0x10000001);b.write32(0x1004,instruction(35,1,2));c.step();c.step();ck(c.cop0[14]==0x1000&&(c.cop0[13]&0x80000000u),"delay-slot user load records branch EPC/BD");
 std::cout<<"PASS: PS1 user fetch/data permissions, isolated stores, COP0 permissions/RI, LWC0/SWC0, COP2 priority, pending load and delay-slot faults\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
