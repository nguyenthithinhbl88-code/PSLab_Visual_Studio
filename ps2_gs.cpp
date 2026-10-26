#include "ps2_gs.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
namespace ps2 {
namespace {
unsigned index(uint32_t a,unsigned n){
 if((n!=4&&n!=8)||(a&(n-1)))throw std::runtime_error("GS supports aligned word/doubleword register access only");
 for(unsigned i=0;i<Gs::addresses.size();i++)if(a>=Gs::addresses[i]&&uint64_t(a)+n<=uint64_t(Gs::addresses[i])+8)return i;
 throw std::runtime_error("GS register address not implemented");
}
}
void Gs::reset(){regs.fill(0);drawing.fill(0);gif_tag_low=gif_tag_high=gif_words=gif_packets=0;gif_remaining=gif_cursor=0;gif_format=gif_nreg=0;gif_faulted=false;gif_q=0x3f800000;std::fill(vram.begin(),vram.end(),0);vertices={};vertex_count=0;primitives=pixels=skipped_primitives=transfer_pixels=0;render_warning.clear();transfer_count=transfer_total=transfer_width=transfer_x=transfer_y=transfer_base=transfer_bw=0;regs[15]=0x4000;regs[16]=0x7f00;resets++;}
uint64_t Gs::read(uint32_t a,unsigned n)const{auto i=index(a,n);return (regs[i]>>((a-addresses[i])*8))&(n==4?0xffffffffull:~0ull);}
void Gs::write(uint32_t a,uint64_t v,unsigned n){
 auto i=index(a,n);const auto shift=(a-addresses[i])*8;const uint64_t mask=n==4?0xffffffffull<<shift:~0ull;
 v=(v<<(shift))&mask;writes++;
 if(i==15){if(v&0x200){reset();return;}regs[i]&=~(v&31);return;}
 if(i==18)return; // SIGLBLID is written through GIF SIGNAL/LABEL.
 auto merged=(regs[i]&~mask)|v;
 if(i==16)merged=(merged&0x1f00)|0x6000;
 if(i==0)merged&=0xffff;
 if(i==2)merged&=15;
 if(i==13||i==17)merged&=1;
 if(i==14)merged&=0xffffff;
 regs[i]=merged;
}
const char* Gs::drawing_name(unsigned id){
 switch(id){
 case 0x00:return "PRIM";
 case 0x01:return "RGBAQ";
 case 0x02:return "ST";
 case 0x03:return "UV";
 case 0x04:return "XYZF2";
 case 0x05:return "XYZ2";
 case 0x06:return "TEX0_1";
 case 0x07:return "TEX0_2";
 case 0x08:return "CLAMP_1";
 case 0x09:return "CLAMP_2";
 case 0x0a:return "FOG";
 case 0x0c:return "XYZF3";
 case 0x0d:return "XYZ3";
 case 0x14:return "TEX1_1";
 case 0x15:return "TEX1_2";
 case 0x16:return "TEX2_1";
 case 0x17:return "TEX2_2";
 case 0x18:return "XYOFFSET_1";
 case 0x19:return "XYOFFSET_2";
 case 0x1a:return "PRMODECONT";
 case 0x1b:return "PRMODE";
 case 0x1c:return "TEXCLUT";
 case 0x22:return "SCANMSK";
 case 0x34:return "MIPTBP1_1";
 case 0x35:return "MIPTBP1_2";
 case 0x36:return "MIPTBP2_1";
 case 0x37:return "MIPTBP2_2";
 case 0x3b:return "TEXA";
 case 0x3d:return "FOGCOL";
 case 0x3f:return "TEXFLUSH";
 case 0x40:return "SCISSOR_1";
 case 0x41:return "SCISSOR_2";
 case 0x42:return "ALPHA_1";
 case 0x43:return "ALPHA_2";
 case 0x44:return "DIMX";
 case 0x45:return "DTHE";
 case 0x46:return "COLCLAMP";
 case 0x47:return "TEST_1";
 case 0x48:return "TEST_2";
 case 0x49:return "PABE";
 case 0x4a:return "FBA_1";
 case 0x4b:return "FBA_2";
 case 0x4c:return "FRAME_1";
 case 0x4d:return "FRAME_2";
 case 0x4e:return "ZBUF_1";
 case 0x4f:return "ZBUF_2";
 case 0x50:return "BITBLTBUF";
 case 0x51:return "TRXPOS";
 case 0x52:return "TRXREG";
 case 0x53:return "TRXDIR";
 case 0x54:return "HWREG";
 case 0x60:return "SIGNAL";
 case 0x61:return "FINISH";
 case 0x62:return "LABEL";
 case 0x0f:case 0x7f:return "NOP";
 default:return nullptr;
 }
}
void Gs::draw_write(unsigned id,uint64_t v){
 if(!drawing_name(id))throw std::runtime_error("GS A+D register unsupported");
 if(id==0x54){transfer_word(v);drawing[id]=v;return;}
 if(id==0x53)transfer_start(v);
 if(id==0x60&&(regs[15]&1))throw std::runtime_error("GS second SIGNAL pending; GIF stall queue not implemented");
 if(id==0x0f||id==0x7f)return;
 if(id==0x60||id==0x62){unsigned shift=id==0x60?0:32;uint32_t mask=uint32_t(v>>32),old=uint32_t(regs[18]>>shift);uint32_t value=(old&~mask)|(uint32_t(v)&mask);regs[18]=(regs[18]&~(0xffffffffull<<shift))|(uint64_t(value)<<shift);}
 if(id==0x60)event(1);
 if(id==0x61)event(2); // Immediate only: this core has no outstanding draw jobs.
 drawing[id]=v;
 if(id==0)vertex_count=0;
 if(id==4||id==5||id==12||id==13)vertex(v,id==4||id==5);
}
void Gs::gif_push(uint64_t low,uint64_t high){
 if(gif_faulted)throw std::runtime_error("GIF stopped after unsupported packet; reset required");
 try{
  if(!gif_remaining){
   unsigned format=unsigned((low>>58)&3),nr=unsigned(low>>60);if(!nr)nr=16;
   if(format>=2&&transfer_count>=transfer_total)throw std::runtime_error("GIF IMAGE needs active PSMCT32 host-to-local transfer");
   if(format==0){for(unsigned i=0;i<nr;i++){auto id=(high>>(i*4))&15;if(id==11)throw std::runtime_error("GIF PACKED descriptor 0xB is reserved");}}
   if(format==1){for(unsigned i=0;i<nr;i++){auto id=unsigned((high>>(i*4))&15);if(id!=15&&!drawing_name(id))throw std::runtime_error("GIF REGLIST register unsupported");}}
   gif_tag_low=low;gif_tag_high=high;gif_format=format;gif_nreg=nr;gif_cursor=0;gif_remaining=uint32_t(low&0x7fff)*(format>=2?1:nr);gif_packets++;gif_q=0x3f800000;
   if(format==0&&(low&(1ull<<46)))draw_write(0,(low>>47)&0x7ff);
  }else if(gif_format>=2){
   transfer_word(low);transfer_word(high);gif_remaining--;gif_cursor++;
  }else{
   auto consume=[&](uint64_t value,uint64_t upper){unsigned id=unsigned((gif_tag_high>>((gif_cursor%gif_nreg)*4))&15);
    if(gif_format==0){
     if(id==14)draw_write(unsigned(upper&255),value);
     else if(id==0)draw_write(0,value&0x7ff);
     else if(id==1)draw_write(1,(uint64_t(gif_q)<<32)|(value&255)|(((value>>32)&255)<<8)|((upper&255)<<16)|(((upper>>32)&255)<<24));
     else if(id==2){draw_write(2,value);gif_q=uint32_t(upper);drawing[1]=(drawing[1]&0xffffffff)|(uint64_t(gif_q)<<32);}
     else if(id==3)draw_write(3,(value&0x3fff)|(((value>>32)&0x3fff)<<16));
     else if(id==4||id==5||id==12||id==13){
      bool fog=id==4||id==12,skip=id>=12||(upper&(1ull<<47));
      uint64_t xy=(value&65535)|(((value>>32)&65535)<<16);
      uint64_t z=fog?(((upper>>4)&0xffffff)<<32)|(((upper>>36)&255)<<56):uint64_t(uint32_t(upper))<<32;
      draw_write(fog?(skip?12:4):(skip?13:5),xy|z);
     }else if(id==10)draw_write(10,((upper>>36)&255)<<56);
     else if(id!=15)draw_write(id,value);
    }else if(id!=15)draw_write(id,value);
    gif_cursor++;gif_remaining--;
   };
   consume(low,high);if(gif_format==1&&gif_remaining)consume(high,0);
  }
  gif_words++;
 }catch(...){gif_faulted=true;throw;}
}
std::string Gs::diagnostic()const{std::ostringstream o;o<<"GS basic software renderer IRQ="<<irq()<<" writes="<<writes<<" resets="<<resets<<"\r\n";for(unsigned i=0;i<regs.size();i++)o<<names[i]<<" ["<<std::hex<<addresses[i]<<"]="<<std::setw(16)<<std::setfill('0')<<regs[i]<<"\r\n";o<<"GIF PATH3 qwords="<<std::dec<<gif_words<<" packets="<<gif_packets<<" remaining="<<gif_remaining<<" stopped="<<gif_faulted<<"\r\n";
 o<<"GS primitives="<<std::dec<<primitives<<" pixels="<<pixels<<" skipped="<<skipped_primitives<<" uploaded_pixels="<<transfer_pixels<<"\r\nRenderer warning: "<<render_warning<<"\r\n";
 for(unsigned i=0;i<drawing.size();i++)if(drawing_name(i))o<<drawing_name(i)<<" ["<<std::hex<<i<<"]="<<std::setw(16)<<drawing[i]<<"\r\n";
 return o.str();}
}
