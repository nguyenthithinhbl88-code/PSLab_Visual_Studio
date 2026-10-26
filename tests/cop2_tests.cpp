#include "core.hpp"
#include "runner.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
void ck(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
uint32_t cop(unsigned rs,unsigned rt,unsigned rd){return 0x48000000|(rs<<21)|(rt<<16)|(rd<<11);}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);c.reset(0x1000);c.cop0[12]|=1u<<30;
 auto code=[&](unsigned i,uint32_t v){b.write32(0x1000+i*4,v);};
 c.r[8]=0x12348000;code(0,cop(4,8,1));code(1,cop(0,9,1));code(2,0x01205021);code(3,0x01205821);
 for(unsigned i=0;i<4;i++)c.step();
 ck(c.gte.read_data(1)==0xffff8000&&c.r[9]==0xffff8000&&c.r[10]==0&&c.r[11]==0xffff8000,"MTC2/MFC2 and real GPR load delay");
 code(4,cop(6,8,26));code(5,cop(2,12,26));code(6,0);c.step();c.step();c.step();ck(c.r[12]==0xffff8000,"CTC2/CFC2 signed H readback");
 c.r[13]=0x2000;b.write32(0x2000,0x11223344);code(7,0xc9a60000);code(8,0xe9a60004);c.step();c.step();ck(c.gte.read_data(6)==0x11223344&&b.read32(0x2004)==0x11223344,"LWC2/SWC2 memory transfer");
 c.gte.write_data(9,3);c.gte.write_data(10,4);c.gte.write_data(11,5);code(9,0x4a000028);c.step();ck(c.gte.read_data(9)==9&&c.gte.read_data(10)==16&&c.gte.read_data(11)==25&&c.reserved_exceptions==0,"COP2 SQR executes");
 // Disabled CU2 is Coprocessor Unusable, with CE=2, rather than Reserved Instruction.
 c.reset(0x1000);code(0,cop(4,8,0));c.step();ck(((c.cop0[13]>>2)&31)==11&&((c.cop0[13]>>28)&3)==2&&c.first_fault.opcode==cop(4,8,0),"CU2 permission and fault opcode");
 // Unaligned LWC2 must leave GTE data unchanged and set BadVAddr.
 c.reset(0x1000);c.cop0[12]|=1u<<30;c.r[13]=0x2001;code(0,0xc9a60000);c.step();ck(c.first_fault.code==4&&c.cop0[8]==0x2001&&c.gte.read_data(6)==0,"unaligned LWC2 exception");
 // Recreate a COP2 instruction at the PC from the user's BIOS fault, without BIOS bytes.
 c.reset(0x8004b70c);c.cop0[12]=0x40000404;c.r[8]=0x1234;b.write32(0x8004b70c,cop(4,8,30));c.step();ck(c.pc==0x8004b710&&c.reserved_exceptions==0&&c.gte.read_data(30)==0x1234,"synthetic COP2 at reported BIOS PC");
 // Unknown opcode snapshot survives thousands of later kernel instructions.
 c.reset(0x8004b70c);c.cop0[12]=0x40000404;c.r[8]=0xabcdef01;b.write32(0x8004b70c,0xffffffff);c.step();
 ck(c.first_fault.valid&&c.first_fault.code==10&&c.first_fault.opcode==0xffffffff&&c.first_fault.pc==0x8004b70c&&c.first_fault.registers[8]==0xabcdef01,"first RI captures opcode and registers");
 b.write32(0x80000080,0x08000020);b.write32(0x80000084,0);for(unsigned i=0;i<2000;i++)c.step();
 ck(c.first_fault.pc==0x8004b70c&&c.first_fault.opcode==0xffffffff,"first fault not overwritten by later trace");
 c.reset(0x1000);c.cop0[12]|=1u<<30;code(0,0x4a000002);c.step();ck(c.first_fault.code==10&&c.first_fault.opcode==0x4a000002,"unsupported GTE opcode records exact fault");
 // Coprocessor branches have a real CPU branch delay slot.
 c.reset(0x1000);c.cop0[12]|=1u<<30;code(0,0x49000002);code(1,0x24080001);code(2,0x24080002);code(3,0x24090003);c.step();c.step();c.step();ck(c.r[8]==1&&c.r[9]==3,"BC2F always branches with delay slot");
 // Fault snapshots preserve the branch EPC and actual delay-slot opcode separately.
 c.reset(0x1000);code(0,0x10000002);code(1,0xffffffff);c.step();c.step();ck(c.first_fault.in_delay&&c.first_fault.pc==0x1004&&c.first_fault.epc==0x1000&&(c.first_fault.cause&0x80000000u),"delay-slot fault snapshot");
 c.reset(0x1000);code(0,0x44000000);c.step();ck(c.first_fault.code==11&&((c.cop0[13]>>28)&3)==1,"absent COP1 CE field");
 // Synthetic kernel routes a reserved instruction into BIOS SystemError A0(40).
 ps1::Bus fatal;ps1::Cpu f(fatal);f.reset(0x1000);f.cop0[12]=0;
 fatal.write32(0x1000,0xffffffff);fatal.write32(0x80,0x24090040);fatal.write32(0x84,0x08000028);fatal.write32(0x88,0);
 fatal.write32(0xa0,0x08000800);fatal.write32(0xa4,0);fatal.write32(0x2000,0x08000800);fatal.write32(0x2004,0);
 std::mutex machine;ps1::Runner runner(f,machine);runner.start(false);
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
 while(runner.active()&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
 runner.stop();ck(f.bios_system_error&&f.instructions<=10&&f.first_fault.pc==0x1000,"fatal BIOS loop pauses worker promptly");
 ck(runner.error().find("opcode=0xffffffff")!=std::string::npos&&f.diagnostic().find("FIRST CPU FAULT")!=std::string::npos,"fatal message and saved first opcode");
 f.reset();ck(!f.bios_system_error&&!f.first_fault.valid&&f.gte.commands==0,"reset clears fatal/snapshot/GTE state");
 std::cout<<"PASS: COP2 transfers/load delay, LWC2/SWC2, CU2/CE, alignment, commands, reported-PC regression, first-fault retention and BC2 branch delay\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
