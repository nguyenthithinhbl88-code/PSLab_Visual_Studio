#include "ps2_ee.hpp"
#include <sstream>
#include <stdexcept>
namespace ps2 {
bool Ee::cop0_supported(unsigned reg)const{
 switch(reg){case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 8:case 9:case 10:case 11:case 12:case 13:case 14:case 15:case 16:case 23:case 28:case 29:case 30:return true;default:return false;}
}
void Ee::cop0_write(unsigned reg,uint32_t v){
 switch(reg){
 case 0:cop0[0]=v&63;return;
 case 1:case 8:case 15:case 23:return; // Hardware-owned registers.
 case 2:cop0[2]=v&0x83ffffff;return;
 case 3:cop0[3]=v&0x03ffffff;return;
 case 4:cop0[4]=(cop0[4]&0x007ffff0)|(v&0xff800000);return;
 case 5:cop0[5]=v&0x01ffe000;return;
 case 6:
  if((v&63)>47)throw std::runtime_error("EE Wired above 47: unsupported/undefined Random range");
  cop0[6]=v&63;cop0[1]=47;random_reloaded=true;return;
 case 10:cop0[10]=v&0xffffe0ff;return;
 case 11:cop0[13]&=~0x8000u;cop0[11]=v;return;
 case 13:cop0[13]=(cop0[13]&~0x300u)|(v&0x300);return;
 default:if(!cop0_supported(reg))throw std::runtime_error("EE COP0 register unsupported");cop0[reg]=v;return;
 }
}
bool Ee::tlb_operation(uint32_t ins){
 if(ins!=0x42000001&&ins!=0x42000002&&ins!=0x42000006&&ins!=0x42000008)return false;
 if(ins==0x42000008){
  cop0[0]=0x80000000;
  for(unsigned i=0;i<tlb.size();i++){
   const auto& entry=tlb[i];if(!entry.written)continue;
   uint32_t compare=~(entry.mask|0x1fffu);
   bool global=(entry.lo0&entry.lo1&1)!=0;
   if(((entry.hi^cop0[10])&compare)==0&&(global||((entry.hi^cop0[10])&255)==0)){cop0[0]=i;break;}
  }
  return true;
 }
 unsigned index=(ins==0x42000006?cop0[1]:cop0[0])&63;
 if(index>=tlb.size())throw std::runtime_error("EE TLB index outside 48-entry bank");
 auto& entry=tlb[index];
 if(ins==0x42000001){
  cop0[5]=entry.mask;cop0[10]=entry.hi;
  unsigned global=(entry.lo0&entry.lo1&1);
  cop0[2]=(entry.lo0&~1u)|global;cop0[3]=(entry.lo1&~1u)|global;
 }else{
  entry={cop0[5],cop0[10]&~cop0[5],cop0[2],cop0[3],true};
 }
 return true;
}
std::string Ee::tlb_diagnostic()const{
 std::ostringstream o;o<<"TLB register/probe model ONLY; memory access still uses flat aliases, not TLB translation/permission/refill.\r\n";
 for(unsigned i=0;i<tlb.size();i++){const auto& t=tlb[i];if(t.written)o<<"TLB["<<std::dec<<i<<"] MASK="<<std::hex<<t.mask<<" HI="<<t.hi<<" LO0="<<t.lo0<<" LO1="<<t.lo1<<"\r\n";}
 return o.str();
}
}
