#include "ps2_ee.hpp"
#include <sstream>
#include <stdexcept>
namespace ps2 {
void Ee::require_cop1()const{
 if(!(cop0[12]&0x20000000))throw std::runtime_error("EE COP1 unusable (Status.CU1=0); coprocessor exception entry unimplemented");
}
bool Ee::cop1_transfer(uint32_t ins){
 require_cop1();
 const unsigned format=(ins>>21)&31,rt=(ins>>16)&31,fs=(ins>>11)&31,fd=(ins>>6)&31;
 auto put=[&](uint32_t v){if(rt)r[rt].low=uint64_t(int64_t(int32_t(v)));};
 if(format==16){
  if((ins&63)!=6||rt)return false; // MOV.S has ft=0 and copies raw bits, including non-IEEE patterns.
  fpr[fd]=fpr[fs];return true;
 }
 if(ins&0x7ff)return false;
 switch(format){
 case 0:put(fpr[fs]);return true; // MFC1: sign extend into GPR low 64, preserve upper 64.
 case 4:fpr[fs]=uint32_t(r[rt].low);return true;
 case 2:if(fs!=0&&fs!=31)return false;put(fs==0?fcr0:fcr31);return true;
 case 6:
  if(fs==0)return true; // FCR0 is read-only.
  if(fs!=31)return false;
  // EE control bits: C=23, cause=14..17, sticky=3..6; fixed bits 24 and 0 are one.
  fcr31=(uint32_t(r[rt].low)&0x0083c078u)|0x01000001u;return true;
 default:return false;
 }
}
std::string Ee::cop1_diagnostic()const{
 std::ostringstream o;o<<"COP1 raw register/memory moves only; arithmetic/conversion/compare/ACC absent.\r\n";
 o<<"FCR0="<<std::hex<<fcr0<<" FCR31="<<fcr31<<" CU1="<<bool(cop0[12]&0x20000000)<<"\r\n";
 for(unsigned i=0;i<32;i++)o<<"f"<<std::dec<<i<<"="<<std::hex<<fpr[i]<<"\r\n";
 return o.str();
}
}
