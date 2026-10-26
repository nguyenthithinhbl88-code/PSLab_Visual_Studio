#include "ps2_ee.hpp"
#include <sstream>
#include <stdexcept>
namespace ps2 {
namespace {
constexpr uint32_t vu_bases[]={0x11000000,0x11004000,0x11008000,0x1100c000};
bool lane(uint32_t a,unsigned n){return (n==1||n==2||n==4)&&!(a&(n-1))&&(a&3)+n<=4;}
uint32_t mask(unsigned n){return n==4?0xffffffffu:(1u<<(8*n))-1;}
uint32_t extract(uint32_t v,uint32_t a,unsigned n){return (v>>((a&3)*8))&mask(n);}
uint32_t merge(uint32_t old,uint32_t v,uint32_t a,unsigned n){unsigned shift=(a&3)*8;uint32_t m=mask(n)<<shift;return (old&~m)|((v<<shift)&m);}
int channel(uint32_t a){if(a>=0x1f801080&&a<0x1f8010f0)return int((a-0x1f801080)/16);if(a>=0x1f801500&&a<0x1f801560)return 7+int((a-0x1f801500)/16);return -1;}
uint32_t* dma_reg(Ee::IopDma& d,uint32_t a,int ch){switch(a&12){case 0:return &d.madr;case 4:return &d.bcr;case 8:return &d.chcr;case 12:if(ch==9)return &d.tadr;}return nullptr;}
constexpr uint32_t globals[]={0x1f8010f0,0x1f8010f4,0x1f801570,0x1f801574,0x1f801578,0x1f80157c};
}
const uint8_t* Ee::mapped_boot_memory(uint32_t p)const{
 if(p>=0x1c000000&&uint64_t(p)-0x1c000000<iop_ram.size())return &iop_ram[p-0x1c000000];
 for(unsigned i=0;i<4;i++)if(p>=vu_bases[i]&&uint64_t(p)-vu_bases[i]<vu_memory[i].size())return &vu_memory[i][p-vu_bases[i]];
 return nullptr;
}
uint8_t* Ee::mapped_boot_memory(uint32_t p){return const_cast<uint8_t*>(static_cast<const Ee&>(*this).mapped_boot_memory(p));}
void Ee::reset_iop_registers(){
 iop_boot_1470=0;iop_config={};iop_dma={};iop_dmac={};iop_dmac[0]=0x07777777;
 iop_stat=iop_mask=iop_ctrl=0;iop_register_reads=iop_register_writes=0;
 // Like main RAM, IOP/VU memory survives an EE CPU reset; fresh instances are zeroed.
}
bool Ee::iop_read(uint32_t a,unsigned n,uint32_t& v)const{
 if(!lane(a,n))return false;
 const uint32_t base=a&~3u;
 if(a==0x1f801470&&n==2){v=iop_boot_1470;iop_register_reads++;return true;}
 if(a==0x1f801472&&n==2){v=iop_boot_1472;iop_boot_1472_reads++;iop_register_reads++;return true;}
 for(unsigned i=0;i<iop_config.size();i++)if(base==iop_config_addresses[i]){v=extract(iop_config[i],a,n);iop_register_reads++;return true;}
 if(base==0x1f801070||base==0x1f801074||base==0x1f801078){
  v=extract(base==0x1f801070?iop_stat:base==0x1f801074?iop_mask:iop_ctrl,a,n);
  if(base==0x1f801078&&a==base)iop_ctrl=0;
  iop_register_reads++;return true;
 }
 if(n!=2&&n!=4)return false;
 const int ch=channel(a);
 if(ch>=0){auto d=iop_dma[unsigned(ch)];if(auto reg=dma_reg(d,a,ch)){v=extract(*reg,a,n);iop_register_reads++;return true;}return false;}
 for(unsigned i=0;i<6;i++)if(base==globals[i]){v=extract(iop_dmac[i],a,n);iop_register_reads++;return true;}
 return false;
}
void Ee::update_iop_dma_irq(){
 const bool summary=(iop_dmac[1]&0x8000)||((iop_dmac[5]&1)&&(iop_dmac[1]&0x800000)&&((iop_dmac[1]&0x7f000000)||(iop_dmac[3]&0x3f000000)));
 iop_dmac[1]=(iop_dmac[1]&0x7fffffffu)|(summary?0x80000000u:0);
 if(summary&&!(iop_dmac[5]&2))iop_stat|=8;
}
bool Ee::iop_write(uint32_t a,unsigned n,uint32_t v){
 if(!lane(a,n))return false;
 const uint32_t base=a&~3u;
 if(a==0x1f801470&&n==2){iop_boot_1470=uint16_t(v);iop_register_writes++;return true;}
 if(a==0x1f801472&&n==2){iop_boot_1472=uint16_t(v);iop_boot_1472_writes++;iop_register_writes++;return true;}
 for(unsigned i=0;i<iop_config.size();i++)if(base==iop_config_addresses[i]){iop_config[i]=merge(iop_config[i],v,a,n);iop_register_writes++;return true;}
 if(base==0x1f801070){const unsigned shift=(a&3)*8;iop_stat&=~(mask(n)<<shift)|(v<<shift);iop_register_writes++;return true;}
 if(base==0x1f801074){iop_mask=merge(iop_mask,v,a,n);iop_register_writes++;return true;}
 if(base==0x1f801078){iop_ctrl=merge(iop_ctrl,v,a,n)&1;iop_register_writes++;return true;}
 if(n!=2&&n!=4)return false;
 const int ch=channel(a);
 if(ch>=0){if(auto reg=dma_reg(iop_dma[unsigned(ch)],a,ch)){
   uint32_t value=merge(*reg,v,a,n);const unsigned off=a&12;
   if(off==8&&(value&0x01000000))throw std::runtime_error("IOP DMA start unsupported: configuration registers exist; transfer engine/IOP execution absent");
   if(off==0||off==12)value&=0xffffff;
   *reg=value;iop_register_writes++;return true;
  }return false;
 }
 for(unsigned i=0;i<6;i++)if(base==globals[i]){
  if(i==1||i==3){
   const uint32_t written=(v&mask(n))<<((a&3)*8),lanes=mask(n)<<((a&3)*8);
   const uint32_t controls=i==1?0x00ff807fu:0x003f0610u,flags=i==1?0x7f000000u:0x3f000000u;
   iop_dmac[i]=(iop_dmac[i]&~(controls&lanes))|(written&controls);
   iop_dmac[i]&=~(written&flags);
  }else{iop_dmac[i]=merge(iop_dmac[i],v,a,n);if(i==4)iop_dmac[i]&=1;if(i==5)iop_dmac[i]&=3;}
  iop_register_writes++;update_iop_dma_irq();return true;
 }
 return false;
}
std::string Ee::iop_diagnostic()const{
 std::ostringstream o;o<<"IOP bootstrap register bank: reads="<<iop_register_reads<<" writes="<<iop_register_writes<<"; bus config storage only, no remapping/waitstates.\r\n";
 o<<"IOP_BOOT_1470="<<std::hex<<iop_boot_1470<<" I_STAT="<<iop_stat<<" I_MASK="<<iop_mask<<" I_CTRL="<<iop_ctrl<<" (snapshot does not clear CTRL; IOP CPU absent)\r\n";
 for(unsigned i=0;i<iop_config.size();i++)o<<"IOP_CONFIG["<<std::hex<<iop_config_addresses[i]<<"]="<<iop_config[i]<<"\r\n";
 for(unsigned i=0;i<6;i++)o<<"IOP_DMAC["<<std::hex<<globals[i]<<"]="<<iop_dmac[i]<<"\r\n";
 for(unsigned i=0;i<13;i++){const auto& d=iop_dma[i];o<<"IOP_DMA"<<std::dec<<i<<" MADR="<<std::hex<<d.madr<<" BCR="<<d.bcr<<" CHCR="<<d.chcr;if(i==9)o<<" TADR="<<d.tadr;o<<"\r\n";}
 o<<"IOP RAM 2 MiB and VU code/data 40 KiB mapped for EE access only; no IOP/VU execution.\r\n";return o.str();
}
}
