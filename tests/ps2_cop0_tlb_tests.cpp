#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();
 // Exact failing opcode/PC visible in the user's screenshot; r3=0 initializes Wired.
 e->reset(0xbfc00854);uint32_t instruction=0x40833000;for(unsigned i=0;i<4;i++)e->bios[0x854+i]=uint8_t(instruction>>(i*8));e->r[3].low=0;e->step();
 check(e->pc==0xbfc00858&&e->cop0[6]==0&&e->cop0[1]==47&&e->instructions==1&&e->first_fault.empty(),"screenshot MTC0 Wired stop fixed");
 auto one=[&](uint32_t op){e->pc=0x100000;e->next=e->pc+4;e->delay=false;e->write(e->pc,op,4);e->step();};
 e->reset();e->r[3].low=40;one(0x40833000);for(unsigned i=0;i<100;i++){one(0);check(e->cop0[1]>=40&&e->cop0[1]<=47,"Random stays above Wired");}
 auto before=e->cop0[1];e->cop0_write(1,0);check(e->cop0[1]==before,"Random read-only");
 e->cop0_write(0,5);e->cop0_write(5,0);e->cop0_write(10,0x00400027);e->cop0_write(2,0x12347);e->cop0_write(3,0x56787);one(0x42000002);
 check(e->tlb[5].written&&e->tlb[5].hi==0x400027,"TLBWI indexed register storage");
 e->cop0_write(10,0x00400055);one(0x42000008);check(e->cop0[0]==5,"global entry ignores ASID when both G bits set");
 e->cop0_write(2,0);e->cop0_write(3,0);e->cop0_write(5,0);one(0x42000001);check(e->cop0[2]==0x12347&&e->cop0[3]==0x56787&&e->cop0[10]==0x400027,"TLBR staging roundtrip");
 e->cop0_write(0,6);e->cop0_write(10,0x00600011);e->cop0_write(2,0x1007);e->cop0_write(3,0x2006);one(0x42000002);e->cop0_write(10,0x00600012);one(0x42000008);check(e->cop0[0]==0x80000000,"non-global ASID miss");
 e->cop0_write(10,0x00600011);one(0x42000008);check(e->cop0[0]==6,"non-global ASID match");one(0x42000001);check(!(e->cop0[2]&1)&&!(e->cop0[3]&1),"TLBR reads combined Global bit");
 e->cop0_write(0,7);e->cop0_write(5,0x6000);e->cop0_write(10,0x00806011);e->cop0_write(2,7);e->cop0_write(3,7);one(0x42000002);e->cop0_write(10,0x00802055);one(0x42000008);check(e->cop0[0]==7,"PageMask-aware probe");
 unsigned random=e->cop0[1];one(0x42000006);check(e->tlb[random].written,"TLBWR uses pre-retirement Random index");
 e->cop0_write(2,~0u);e->cop0_write(3,~0u);e->cop0_write(10,~0u);e->cop0_write(5,~0u);check(e->cop0[2]==0x83ffffff&&e->cop0[3]==0x03ffffff&&e->cop0[10]==0xffffe0ff&&e->cop0[5]==0x01ffe000,"staging masks and scratchpad selector bit");
 e->cop0_write(28,0x1234);e->cop0_write(29,0x5678);check(e->cop0[28]==0x1234&&e->cop0[29]==0x5678,"cache tag latches only");
 e->cop0[23]=0x12345678;e->cop0_write(23,0);check(e->cop0[23]==0x12345678,"BadPAddr read-only");
 e->cop0[4]=0x123450;e->cop0_write(4,0xffffffff);check(e->cop0[4]==0xff923450,"Context preserves hardware BadVPN field");
 e->cop0_write(0,63);bool failed=false;try{one(0x42000002);}catch(const std::runtime_error&){failed=true;}check(failed&&e->pc==0x100000,"invalid TLB index stops without out-of-bounds access");
 e->reset();failed=false;try{e->cop0_write(6,63);}catch(const std::runtime_error&){failed=true;}check(failed&&e->cop0[6]==0,"undefined Wired range rejected atomically");
 check(!e->tlb[5].written&&e->cop0[1]==47,"reset clears staged TLB bank");
 std::cout<<"PASS: screenshot Wired regression, Random bounds/read-only, TLBWI/WR/R/P, ASID/global/page masks, CP0 staging masks and strict bounds\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}}
