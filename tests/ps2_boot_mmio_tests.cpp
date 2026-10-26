#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> std::string failure(F call){try{call();}catch(const std::runtime_error& e){return e.what();}throw std::runtime_error("Expected a failure");}
}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();
 // Replay only PC/opcode pairs supplied in the uploaded trace, not a BIOS image.
 const uint32_t trace[][2]={
  {0xbfc00000,0x401a7800},
  {0xbfc00004,0x00000000},
  {0xbfc00008,0x2b410059},
  {0xbfc0000c,0x14200005},
  {0xbfc00010,0x00000000},
  {0xbfc00014,0x3c1abfc0},
  {0xbfc00018,0x375a0800},
  {0xbfc0001c,0x03400008},
  {0xbfc00020,0x00000000},
  {0xbfc00800,0x3c1a0007},
  {0xbfc00804,0x375a3003},
  {0xbfc00808,0x409a8000},
  {0xbfc0080c,0x0000040f},
  {0xbfc00810,0x3c1a7040},
  {0xbfc00814,0x409a6000},
  {0xbfc00818,0x0000040f},
  {0xbfc0081c,0x40804800},
  {0xbfc00820,0x241a0001},
  {0xbfc00824,0x409a5800},
  {0xbfc00828,0x0000040f},
  {0xbfc0082c,0x2402ffff},
  {0xbfc00830,0x3c03b000},
  {0xbfc00834,0x3463f500},
  {0xbfc00838,0xac620000},
};
 for(auto& item:trace)for(unsigned b=0;b<4;b++)e->bios[(item[0]&0x3fffff)+b]=uint8_t(item[1]>>(8*b));
 e->reset(0xbfc00000);
 for(auto& item:trace){check(e->pc==item[0],"Replay PC differs from user trace");e->step();}
 check(e->pc==0xbfc0083c&&e->instructions==24&&e->boot_f500==0xffffffff&&e->boot_f500_writes==1&&e->first_fault.empty(),"BIOS store to F500 no longer stops");
 check(e->cop0[12]==0x70400000&&e->cop0[16]==0x73003&&e->cop0[9]==8,"BIOS replay COP0 state");
 check(e->read(0x1000f500,4)==0xffffffff&&e->read(0xb000f500,4)==0xffffffff&&e->read(0x9000f500,4)==0xffffffff,"F500 physical/KSEG aliases");
 e->write(0x9000f500,0x11223344,4);check(e->read(0xb000f500,4)==0x11223344&&e->boot_f500_writes==2,"F500 word readback");
 auto before=e->boot_f500;
 failure([&]{e->write(0x1000f500,0,1);});failure([&]{e->write(0x1000f500,0,8);});failure([&]{e->write(0x1000f502,0,4);});failure([&]{e->read(0x1000f504,4);});
 check(e->boot_f500==before&&e->boot_f500_writes==2,"Invalid F500 accesses preserve latch");
 e->reset();check(e->boot_f500==0&&e->boot_f500_writes==0,"Reset clears experimental F500 state");
 auto error=failure([&]{e->write(0xb000f504,1,4);});check(error.find("write")!=std::string::npos&&error.find("b000f504")!=std::string::npos&&error.find("unmapped read")==std::string::npos,"Unknown store reports write and original address");
 auto old=e->bios[0];error=failure([&]{e->write(0xbfc00000,0,4);});check(error.find("write")!=std::string::npos&&e->bios[0]==old,"Read-only BIOS remains unchanged");
 error=failure([&]{e->write128(0xbfc00000,{0,0});});check(error.find("write")!=std::string::npos&&e->bios[0]==old,"Read-only quadword write rejected before mutation");
 e->write128(0x70003ff0,{0x0123456789abcdef,0xfedcba9876543210});check(e->read(0x70003ff0,8)==0x0123456789abcdef&&e->read(0x70003ff8,8)==0xfedcba9876543210,"Scratchpad final quadword remains writable");
 e->write(0x80000200,0x1122334455667788,8);check(e->read(0xa0000200,8)==0x1122334455667788,"RAM write aliases retained");
 check(e->diagnostic().find("BOOT_F500=")!=std::string::npos,"F500 diagnostic exposed");
 std::cout<<"PASS: 24-instruction uploaded BIOS trace through F500, aliases/reset/strict widths, write fault direction, BIOS write protection and RAM/scratchpad writes\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
