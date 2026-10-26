#include "ps2_ee.hpp"
#include <sstream>
#include <stdexcept>
namespace ps2 {
namespace {
bool lane(uint32_t a,unsigned n){return (n==1||n==2||n==4)&&!(a&(n-1))&&(a&3)+n<=4;}
uint32_t mask(unsigned n){return n==4?0xffffffffu:(1u<<(8*n))-1;}
uint32_t extract(uint32_t v,uint32_t a,unsigned n){return (v>>((a&3)*8))&mask(n);}
uint32_t merge(uint32_t old,uint32_t v,uint32_t a,unsigned n){unsigned shift=(a&3)*8;uint32_t m=mask(n)<<shift;return (old&~m)|((v<<shift)&m);}
bool vif_location(uint32_t a,unsigned& channel,unsigned& index){
 if(a<0x10003800||a>=0x10004000||(a&15))return false;
 channel=(a-0x10003800)/0x400;index=((a-0x10003800)%0x400)/16;
 if(index>=24||index==15)return false;
 if(!channel&&(index==10||index==11||index==12||index==14))return false;
 return true;
}
}
void Ee::reset_boot_peripherals(){serial={};vif={};}
bool Ee::serial_read(uint32_t a,unsigned n,uint32_t& v)const{
 if(!lane(a,n))return false;
 const uint32_t base=a&~3u;uint32_t data=0;
 switch(base){
 case 0x1000f100:data=serial.lcr;break;
 case 0x1000f110:data=0;break; // No RX data or simulated line errors.
 case 0x1000f120:data=serial.ier;break;
 case 0x1000f130:data=0;break; // Synchronous TX sink, empty RX; no pending serial IRQ.
 case 0x1000f140:data=serial.fcr;break;
 case 0x1000f150:data=serial.bgr;break;
 case 0x1000f1c0:if(a!=base||n!=1)return false;data=0;break; // Empty RX policy, no fabricated input.
 default:return false;
 }
 v=extract(data,a,n);serial.reads++;return true;
}
bool Ee::serial_write(uint32_t a,unsigned n,uint32_t v){
 if(!lane(a,n))return false;
 const uint32_t base=a&~3u;
 if(base==0x1000f180){
  if(a!=base)return false;
  // Non-byte TX is a little-endian compatibility policy, not hardware-verified.
  for(unsigned i=0;i<n;i++){
   const uint8_t c=uint8_t(v>>(i*8));
   if(console.size()==16384)console.erase(0,1);
   console+=(c=='\n'||c=='\r'||c=='\t'||(c>=32&&c<127))?char(c):'.';
  }
  serial.tx_bytes+=n;serial.writes++;return true;
 }
 switch(base){
 case 0x1000f100:serial.lcr=merge(serial.lcr,v,a,n);break;
 case 0x1000f110:break; // Status is read-only.
 case 0x1000f120:serial.ier=merge(serial.ier,v,a,n);break;
 case 0x1000f130:break; // IRQ acknowledgement has nothing pending in this model.
 case 0x1000f140:
  serial.fcr=merge(serial.fcr,v,a,n);
  if((serial.fcr&1)&&(serial.fcr&6))serial.reset_requests++;
  break; // There is no buffered TX/RX to discard; captured console history remains.
 case 0x1000f150:serial.bgr=merge(serial.bgr,v,a,n);break;
 default:return false;
 }
 serial.writes++;return true;
}
bool Ee::vif_read(uint32_t a,uint32_t& v)const{
 unsigned channel,index;if(!vif_location(a,channel,index))return false;
 v=index==1?0:vif[channel].regs[index];return true; // FBRST read policy: zero.
}
bool Ee::vif_write(uint32_t a,uint32_t v){
 unsigned channel,index;if(!vif_location(a,channel,index))return false;
 auto& state=vif[channel];auto& regs=state.regs;
 switch(index){
 case 0:if(channel)regs[0]=(regs[0]&~0x800000u)|(v&0x800000);break;
 case 1:
  if(v&1){state={};return true;}
  if(v&2)regs[0]|=0x200;
  if(v&4)regs[0]|=0x100; // No executing command: STOP is immediate.
  if(v&8)regs[0]&=~0x3f00u;
  break;
 case 2:regs[2]=v&7;break;
 case 3:regs[3]=v&0xffff;regs[0]&=~0x40u;break;
 default:break; // CYCLE/MODE/CODE/ROW/COL etc. are command-produced read-only state.
 }
 return true;
}
void Ee::vif_push(unsigned channel,Register value){
 if(channel>=2)throw std::runtime_error("Invalid VIF channel");
 struct MemoryWrite {unsigned region,offset;uint32_t value;};
 std::vector<MemoryWrite> memory_writes;
 auto state=vif[channel]; // Validate a full quadword before committing any register/payload progress.
 if(state.regs[0]&(0x3f00u|0x800000u))throw std::runtime_error("VIF FIFO stalled or reverse direction; queue/readback engine unsupported");
 const uint32_t words[]={uint32_t(value.low),uint32_t(value.low>>32),uint32_t(value.high),uint32_t(value.high>>32)};
 for(uint32_t word:words){
  if(state.payload){
   if(state.payload_kind){
    unsigned region=channel*2+(state.payload_kind==2?1:0);
    memory_writes.push_back({region,state.memory_at,word});
    state.memory_at=(state.memory_at+4)%unsigned(vu_memory[region].size());
    state.cursor++;
    state.regs[6]=((state.payload+(state.payload_kind==1?0:2))/(state.payload_kind==1?2:4))&255;
   }else state.regs[state.destination+state.cursor++]=word;
   if(!--state.payload){state.cursor=0;state.regs[0]&=~3u;state.regs[6]=0;}
   continue;
  }
  const unsigned cmd=(word>>24)&0x7f;
  if((word&0x80000000u)&&!(state.regs[2]&1))throw std::runtime_error("VIF interrupt-bit command unsupported without MII; VIF IRQ scheduling absent");
  state.payload_kind=0;
  switch(cmd){
  case 0:break;
  case 1:state.regs[4]=word&0xffff;break;
  case 2:
   if(!channel)throw std::runtime_error("OFFSET is VIF1-only");
   state.regs[11]=word&0x3ff;state.regs[0]&=~0x80u;state.regs[12]=state.regs[10];break;
  case 3:if(!channel)throw std::runtime_error("BASE is VIF1-only");state.regs[10]=word&0x3ff;break;
  case 4:state.regs[9]=word&(channel?0x3ff:0xff);break;
  case 5:state.regs[5]=word&3;break;
  case 7:state.regs[3]=word&0xffff;state.regs[0]|=0x40;break;
  case 0x20:state.payload=1;state.destination=7;break;
  case 0x30:state.payload=4;state.destination=16;break;
  case 0x31:state.payload=4;state.destination=20;break;
  case 0x4a:{
   const unsigned count=((word>>16)&255)?((word>>16)&255):256;
   state.payload=count*2;state.payload_kind=1;state.regs[6]=count&255;
   state.memory_at=((word&0xffff)*8)%unsigned(vu_memory[channel*2].size());break;
  }
  case 0x6c:{
   // First UNPACK format: V4-32, no masks/addition/skip/fill. Reject unmodeled modes before mutation.
   const unsigned cycle=state.regs[4];
   if((cycle&255)!=((cycle>>8)&255)||state.regs[5])throw std::runtime_error("VIF UNPACK V4-32 requires equal CL/WL and MODE=0");
   const unsigned count=((word>>16)&255)?((word>>16)&255):256;
   state.payload=count*4;state.payload_kind=2;state.regs[6]=count&255;
   unsigned at=word&0x3ff;if(channel&&(word&0x8000))at+=state.regs[12];
   state.memory_at=(at*16)%unsigned(vu_memory[channel*2+1].size());break;
  }
  default:{std::ostringstream o;o<<"VIF"<<channel<<" command 0x"<<std::hex<<cmd<<" unsupported: partial VIF stream; no VU execution/DIRECT";throw std::runtime_error(o.str());}
  }
  state.regs[8]=word;state.commands++;
  if(state.payload){state.cursor=0;state.regs[0]=(state.regs[0]&~3u)|1;}
 }
 for(const auto& item:memory_writes)for(unsigned n=0;n<4;n++)vu_memory[item.region][item.offset+n]=uint8_t(item.value>>(8*n));
 state.packets++;vif[channel]=state;
}
std::string Ee::peripheral_diagnostic()const{
 std::ostringstream o;o<<"EE SIO: LCR="<<std::hex<<serial.lcr<<" LSR=0 IER="<<serial.ier<<" ISR=0 FCR="<<serial.fcr<<" BGR="<<serial.bgr<<"\r\n";
 o<<"EE SIO reads="<<std::dec<<serial.reads<<" writes="<<serial.writes<<" TX bytes="<<serial.tx_bytes<<" FIFO reset requests="<<serial.reset_requests<<" (synchronous console sink; empty RX; no baud timing/serial IRQ)\r\n";
 for(unsigned n=0;n<2;n++){
  const auto& state=vif[n];o<<"VIF"<<n<<" configuration only: QW="<<state.packets<<" commands="<<state.commands<<" payload words pending="<<state.payload<<"\r\n";
  for(unsigned i=0;i<24;i++){unsigned channel,index;uint32_t a=0x10003800+n*0x400+i*16;if(vif_location(a,channel,index))o<<"VIF["<<std::hex<<a<<"]="<<state.regs[i]<<"\r\n";}
 }
 o<<"VIF supports normal DMA/CPU SQ, config/MPG/limited V4-32 UNPACK; no VU execution/PATH2/other UNPACK formats.\r\n";
 return o.str();
}
}
