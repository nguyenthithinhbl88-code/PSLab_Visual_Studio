#include "ps2_ee.hpp"
#include <stdexcept>
#include <sstream>
namespace ps2 {
namespace {
uint32_t advance(uint32_t address,uint32_t bytes){
 return address&0x80000000u?0x80000000u|((address+bytes)&0x3ff0u):(address+bytes)&0x7ffffff0u;
}
}
Register Ee::dma_read_quadword(uint32_t address,unsigned channel){
 if(address&0x80000000u){const auto at=0x70000000u+(address&0x3ff0);return {read(at,8),read(at+8,8)};}
 if(uint64_t(address)+16>ram.size()){
  dma[channel].chcr&=~0x100u;dmac[1]|=0x8000;
  std::ostringstream o;o<<"EE DMA"<<channel<<" RAM source/tag out of bounds @ 0x"<<std::hex<<address;throw std::runtime_error(o.str());
 }
 return {read(address,8),read(address+8,8)};
}
void Ee::vif_dma_tick(unsigned channel){
 auto& d=dma[channel];
 if(d.qwc){
  auto value=dma_read_quadword(d.madr,channel);
  vif_push(channel,value); // Parser commits before DMA pointers/counters advance.
  d.madr=advance(d.madr,16);d.qwc--;dma_quadwords++;
 }
 if(!d.qwc){d.chcr&=~0x100u;dmac[1]|=1u<<channel;}
}
void Ee::gif_dma_tick(){
 auto& d=dma[2];const bool chain=(d.chcr&12)==4;
 auto complete=[&](){d.chcr&=~0x100u;dmac[1]|=4;};
 if(d.qwc){
  const auto qw=dma_read_quadword(d.madr);gs.gif_push(qw.low,qw.high);
  d.madr=advance(d.madr,16);d.qwc--;dma_quadwords++;
  if(chain&&d.follow_madr)d.tadr=d.madr;
  if(!d.qwc&&(!chain||d.chain_end))complete();
  return;
 }
 if(!chain||d.chain_end){complete();return;}
 // At most one tag per bus tick: even a cyclic zero-QWC chain cannot block the UI.
 const auto tag=dma_read_quadword(d.tadr);
 const unsigned id=unsigned(tag.low>>28)&7,pce=unsigned(tag.low>>26)&3;
 const unsigned asp=(d.chcr>>4)&3,qwc=unsigned(tag.low)&0xffff;
 if(pce==1)throw std::runtime_error("GIF DMAtag reserved PCE value");
 if(id==5&&asp>=2)throw std::runtime_error("GIF DMA CALL exceeds two-entry ASR stack");
 const auto inline_data=advance(d.tadr,16),address=uint32_t(tag.low>>32)&0xfffffff0u;
 uint32_t madr=inline_data,tadr=d.tadr,next_asp=asp,asr0=d.asr0,asr1=d.asr1;
 bool end=false,follow=false;
 switch(id){
 case 0:madr=address;tadr=inline_data;end=true;break;
 case 1:tadr=inline_data;follow=true;break;
 case 2:tadr=address;break;
 case 3:case 4:madr=address;tadr=inline_data;break;
 case 5:
  if(!asp)asr0=advance(inline_data,qwc*16);else asr1=advance(inline_data,qwc*16);
  next_asp++;tadr=address;break;
 case 6:
  if(asp==2){tadr=asr1;next_asp--;}else if(asp==1){tadr=asr0;next_asp--;}else end=true;
  break;
 case 7:end=true;break;
 }
 if((d.chcr&0x80)&&(tag.low&0x80000000u))end=true;
 if(pce==2)dmac[2]&=~0x80000000u;else if(pce==3)dmac[2]|=0x80000000u;
 d.chcr=(d.chcr&0xffffu)|(uint32_t(tag.low)&0xffff0000u);
 d.chcr=(d.chcr&~0x30u)|(next_asp<<4);
 d.qwc=qwc;d.madr=madr;d.tadr=tadr;d.asr0=asr0;d.asr1=asr1;d.chain_end=end;d.follow_madr=follow;dma_tags++;
 if(!qwc&&end)complete();
}
void Ee::chain_demo(){
 gpu_demo();
 // Preserve the authored GS packet, then let EE code start a REF + END chain.
 const unsigned original_count=1+unsigned(read(0x100,8)&0x7fff);
 std::vector<Register> packets;
 for(unsigned i=0;i<original_count;i++){
  if(i==21)packets.push_back({0x8000ff00,1});
  if(i==22)packets.push_back({0x80ff0000,1});
  Register packet{read(0x100+i*16,8),read(0x108+i*16,8)};
  if(i==18)packet.low=3|8; // Triangle now consumes per-vertex RGBAQ colors.
  if(i==19)packet.low=0x800000ff;
  packets.push_back(packet);
 }
 const unsigned count=unsigned(packets.size());packets[0].low=(packets[0].low&~0x7fffull)|(count-1);
 for(unsigned i=0;i<count;i++)write128(0x100+i*16,packets[i]);
 write128(0x1000,{uint64_t(count)|(3ull<<28)|(0x100ull<<32),0});
 write128(0x1010,{7ull<<28,0});
 gs.reset(); // EE instructions below must enable scanout themselves.
 write(0x1100,(639ull<<32)|(447ull<<44),8);
 // EE sets PMODE/DISPFB/DISPLAY, starts the chain, then waits using CPCOND0.
 const uint32_t code[]={0x3c081200,0x24090001,0xfd090000,0x34091400,0xfd090070,0x240a1100,0xdd490000,0xfd090080,
  0x3c081001,0x24090001,0xad09e000,0x24091000,0xad09a030,
  0xad00a020,0x24090004,0xad09e020,0x24090105,0xad09a000,
  0x4100ffff,0,0x08040014,0};
 for(unsigned i=0;i<sizeof(code)/sizeof(code[0]);i++)write(0x100000+i*4,code[i],4);
}
}
