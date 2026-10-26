#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();
 for(unsigned type:{0u,1u,2u,3u,16u,17u,18u,19u})for(uint64_t value:{0ull,1ull,~0ull}){
  e->reset();e->r[1].low=value;e->r[31]={55,99};e->write(e->pc,0x04200002|(type<<16),4);e->write(e->pc+4,0x24020007,4);e->write(e->pc+8,0x24030009,4);e->write(e->pc+12,0,4);
  bool taken=(type&1)?int64_t(value)>=0:int64_t(value)<0;e->step();
  if((type&2)&&!taken)check(e->pc==0x100008&&e->r[2].low==0&&!e->delay,"REGIMM likely annuls delay slot");
  else{check(e->pc==0x100004&&e->delay,"REGIMM delay slot scheduled");e->step();check(e->r[2].low==7&&e->pc==(taken?0x10000c:0x100008),"REGIMM target/fallthrough");}
  check(e->r[31].low==((type&16)?0x100008:55)&&e->r[31].high==99,"REGIMM link and upper GPR preservation");
 }
 for(unsigned opcode:{0x14u,0x15u,0x16u,0x17u})for(uint64_t value:{0ull,1ull,~0ull}){
  e->reset();e->r[1].low=value;e->write(e->pc,(opcode<<26)|(1u<<21)|2,4);e->write(e->pc+4,0x24020007,4);bool taken=opcode==0x14?value==0:opcode==0x15?value!=0:opcode==0x16?int64_t(value)<=0:int64_t(value)>0;e->step();
  if(taken){e->step();check(e->r[2].low==7&&e->pc==0x10000c,"likely taken delay slot");}else check(e->r[2].low==0&&e->pc==0x100008,"likely not taken annul");
 }
 e->reset();e->r[1].low=64;e->r[2].low=0x8000000000000001ull;e->r[3].high=123;e->write(e->pc,(1u<<21)|(2u<<16)|(3u<<11)|0x14,4);e->step();check(e->r[3].low==e->r[2].low&&e->r[3].high==123,"DSLLV shift mask and upper preservation");
 e->reset();e->r[1].low=63;e->r[2].low=0x8000000000000000ull;e->write(e->pc,(1u<<21)|(2u<<16)|(3u<<11)|0x17,4);e->step();check(e->r[3].low==~0ull,"DSRAV signed shift");
 e->reset();e->write(0x200,0x87654321,4);e->write(e->pc,0x9c020200,4);e->step();check(e->r[2].low==0x87654321,"LWU zero extends");
 std::cout<<"PASS: all 8 REGIMM branches, 4 branch-likely families, delay annul/link/source edges, variable 64-bit shifts and LWU\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}}
