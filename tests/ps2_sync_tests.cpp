#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();
 // Replay only the instruction/address pairs reported in the user log, not a BIOS image.
 const uint32_t addresses[]={0xbfc00000,0xbfc00004,0xbfc00008,0xbfc0000c,0xbfc00010,0xbfc00014,0xbfc00018,0xbfc0001c,0xbfc00020,0xbfc00800,0xbfc00804,0xbfc00808,0xbfc0080c};
 const uint32_t words[]={0x401a7800,0,0x2b410059,0x14200005,0,0x3c1abfc0,0x375a0800,0x03400008,0,0x3c1a0007,0x375a3003,0x409a8000,0x0000040f};
 for(unsigned i=0;i<13;i++)for(unsigned b=0;b<4;b++)e->bios[(addresses[i]&0x3fffff)+b]=uint8_t(words[i]>>(8*b));
 e->reset(0xbfc00000);
 for(unsigned i=0;i<13;i++){check(e->pc==addresses[i],"BIOS trace PC differs");e->step();}
 check(e->pc==0xbfc00810&&e->next==0xbfc00814&&e->instructions==13&&e->cop0[16]==0x73003&&e->first_fault.empty(),"reported SYNC.P stop resolved");
 e->demo();e->r[7]={0x123456789abcdef0,0xabcdef0123456789};e->write(0x100000,0x0000000f,4);e->write(0x100004,0x0000040f,4);e->step();e->step();
 check(e->pc==0x100008&&e->instructions==2&&e->cop0[9]==2&&e->r[7].low==0x123456789abcdef0&&e->r[7].high==0xabcdef0123456789,"SYNC.L/P preserve GPRs and advance counters");
 // Synchronous RAM stores before a barrier remain visible afterwards.
 e->demo();e->write(0x100000,0xac020200,4);e->write(0x100004,0x0000040f,4);e->write(0x100008,0x8c030200,4);e->r[2].low=0x87654321;
 for(unsigned i=0;i<3;i++){e->step();}check(e->r[3].low==0xffffffff87654321ull,"store/SYNC/load visibility");
 // Unknown SPECIAL instructions still stop rather than being silently skipped.
 e->demo();e->write(0x100000,0x0000000d,4);bool failed=false;try{e->step();}catch(const std::runtime_error&){failed=true;}check(failed&&e->pc==0x100000&&!e->first_fault.empty(),"unknown SPECIAL retains first fault");
 std::cout<<"PASS: uploaded 13-instruction BIOS trace through SYNC.P, SYNC.L/P state/counters, memory visibility and strict unsupported instructions\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
