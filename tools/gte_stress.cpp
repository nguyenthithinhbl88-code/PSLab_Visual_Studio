#include "gte.hpp"
#include <stdexcept>
#include <iostream>
int main(){try{
 uint32_t seed=0x50533118;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
 const unsigned commands[]={1,6,12,16,17,18,19,20,22,27,28,30,32,40,41,42,45,46,48,61,62,63};
 for(unsigned trial=0;trial<2000;trial++){
  ps1::Gte g;for(unsigned i=0;i<32;i++){g.write_data(i,random());g.write_control(i,random());}
  for(auto code:commands){
   const auto opcode=(random()&0x001ffc00)|code;
   if(!g.execute(opcode))throw std::runtime_error("documented opcode rejected");
   for(unsigned i=7;i<=11;i++){auto v=g.read_data(i);if(i==7&&v>65535)throw std::runtime_error("OTZ range");}
   if(g.read_data(19)>65535)throw std::runtime_error("SZ range");
   const auto flag=g.read_control(31);
   if(flag&0xfff)throw std::runtime_error("FLAG reserved bits");
   if(bool(flag&0x80000000u)!=bool(flag&0x7f87e000u))throw std::runtime_error("FLAG summary");
  }
 }
 std::cout<<"PASS: 44000 GTE commands over seeded randomized registers/options; range/FLAG invariants\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
