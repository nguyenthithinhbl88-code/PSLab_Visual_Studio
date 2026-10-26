#include "ps2_ee.hpp"
#include <sstream>
namespace ps2 {
bool Ee::mch_read(uint32_t a,uint32_t& v)const{
 if(a<0x1000f400||a>=0x1000f500||(a&15))return false;
 mch_reads++;
 if(a==0x1000f410||a==0x1000f430){v=0;return true;} // PCSX2 functional busy/status policy; not a calibrated controller.
 if(a!=0x1000f440){v=mch_registers[(a-0x1000f400)/16];return true;}
 const unsigned sub=(mch_ricm>>16)&0xfff,operation=(mch_ricm>>6)&15;
 v=0;if(operation)return true;
 switch(sub){
 case 0x21:if(rdram_device<2){rdram_device++;v=0x1f;}break;
 case 0x23:v=0x0d0d;break;
 case 0x24:v=0x0090;break;
 case 0x40:v=mch_ricm&31;break;
 default:break;
 }
 return true;
}
bool Ee::mch_write(uint32_t a,uint32_t v){
 if(a<0x1000f400||a>=0x1000f500||(a&15))return false;
 mch_writes++;mch_registers[(a-0x1000f400)/16]=v;
 if(a==0x1000f430){
  if(((v>>16)&0xfff)==0x21&&((v>>6)&15)==1&&!(mch_drd&0x80))rdram_device=0;
  mch_ricm=v&0x7fffffff;mch_registers[3]=mch_ricm;
 }else if(a==0x1000f440)mch_drd=v;
 return true;
}
std::string Ee::mch_diagnostic()const{
 std::ostringstream o;o<<"MCH bootstrap block: reads="<<std::dec<<mch_reads<<" writes="<<mch_writes<<" (16 word slots; experimental backing, no physical RDRAM calibration)\r\n";
 for(unsigned i=0;i<16;i++)o<<"MCH["<<std::hex<<(0x1000f400+i*16)<<"]="<<mch_registers[i]<<"\r\n";
 o<<"MCH F410/RICM reads return zero under the functional reference policy; other unidentified slots are storage only.\r\n";return o.str();
}
}
