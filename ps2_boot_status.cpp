#include "ps2_ee.hpp"
#include <sstream>
namespace ps2 {
std::string Ee::boot_status()const{
 std::ostringstream o;
 o<<"Boot display: PMODE="<<std::hex<<gs.regs[0]<<" GIF QW="<<std::dec<<gs.gif_words<<" primitives="<<gs.primitives<<" skipped="<<gs.skipped_primitives<<"\r\n";
 if(!(gs.regs[0]&3))o<<"BIOS has not enabled a GS display circuit. Screen output cannot establish a completed BIOS boot.\r\n";
 else if(!gs.gif_words&&!gs.primitives)o<<"GS display enabled, but no drawing packets/primitives observed.\r\n";
 const uint32_t pattern[]={0x40024800,0x00431023,0x0044102b,0,0,0x1440fffa,0};
 for(unsigned i=0;i+7<=trace.size();i++){
  const auto base=trace.at(i).pc;if(pc<base||pc>base+24)continue;
  bool matches=true;for(unsigned n=0;n<7;n++){const auto& t=trace.at(i+n);if(t.fetch_fault||t.pc!=base+n*4||t.word!=pattern[n]||t.delay!=(n==6)){matches=false;break;}}
  if(matches){o<<"Observed Count polling loop @ "<<std::hex<<base<<": Count="<<cop0[9]<<" start="<<uint32_t(r[3].low)<<" unsigned elapsed="<<uint32_t(cop0[9]-uint32_t(r[3].low))<<" threshold="<<r[4].low<<" (observation only; no loop skipped or register changed)\r\n";break;}
 }
 return o.str();
}
}
