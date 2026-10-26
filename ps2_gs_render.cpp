#include "ps2_gs.hpp"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace ps2 {
// PSMCT32: 64x32 pages, 8x8 blocks, four 8x2 columns per block.
// Express the documented address-bit permutation directly rather than a lookup table.
uint32_t Gs::address32(unsigned base,unsigned bw,unsigned x,unsigned y){
 x&=2047;y&=2047;
 unsigned block=((x>>3)&1)|(((y>>3)&1)<<1)|(((x>>4)&1)<<2)|(((y>>4)&1)<<3)|(((x>>5)&1)<<4);
 unsigned word=(x&1)|((y&1)<<1)|((x&6)<<1)|((y&6)<<3);
 return ((base>>2)+((x>>6)+(y>>5)*bw)*2048+block*64+word)&0xfffff;
}
void Gs::transfer_start(uint64_t dir){
 if((dir&3)==3){transfer_total=transfer_count=0;return;}
 if((dir&3)!=0)throw std::runtime_error("GS only host-to-local transfers supported");
 auto b=drawing[0x50],p=drawing[0x51],s=drawing[0x52];
 unsigned w=unsigned(s&0xfff),h=unsigned((s>>32)&0xfff);
 if(w&&h&&(((b>>56)&63)!=0||((b>>48)&63)==0))throw std::runtime_error("GS upload requires PSMCT32 and nonzero DBW");
 transfer_base=unsigned((b>>32)&0x3fff)*256;transfer_bw=unsigned((b>>48)&63);
 transfer_x=unsigned((p>>32)&2047);transfer_y=unsigned((p>>48)&2047);
 transfer_width=w;transfer_count=0;transfer_total=w*h;
}
void Gs::transfer_word(uint64_t value){
 if(!transfer_total)throw std::runtime_error("GS HWREG needs active PSMCT32 host-to-local transfer");
 for(unsigned i=0;i<2&&transfer_count<transfer_total;i++,transfer_count++){
  unsigned x=transfer_x+transfer_count%transfer_width,y=transfer_y+transfer_count/transfer_width;
  vram[address32(transfer_base,transfer_bw,x,y)]=uint32_t(value>>(32*i));transfer_pixels++;
 }
}
void Gs::vertex(uint64_t v,bool kick){
 const auto prim=drawing[0];unsigned type=unsigned(prim&7);
 const uint64_t mode=(prim&7)|((drawing[0x1a]&1?prim:drawing[0x1b])&0x7f8);
 unsigned context=unsigned((mode>>9)&1);auto offset=drawing[0x18+context];
 Vertex item{int(v&65535)-int(offset&65535),int((v>>16)&65535)-int((offset>>32)&65535),uint32_t(drawing[1])};
 if(type==7){vertex_count=0;skipped_primitives++;render_warning="Reserved primitive type";return;}
 vertices[vertex_count++]=item;
 unsigned needed=type==0?1:(type==1||type==2||type==6?2:3);
 if(vertex_count<needed)return;
 if(kick)rasterize(type,context,mode);
 if(type==2){vertices[0]=vertices[1];vertex_count=1;}
 else if(type==4){vertices[0]=vertices[1];vertices[1]=vertices[2];vertex_count=2;}
 else if(type==5){vertices[1]=vertices[2];vertex_count=2;}
 else vertex_count=0;
}
void Gs::rasterize(unsigned type,unsigned context,uint64_t mode){
 auto frame=drawing[0x4c+context],test=drawing[0x47+context],sc=drawing[0x40+context];
 unsigned psm=unsigned((frame>>24)&63),bw=unsigned((frame>>16)&63);
 auto skip=[&](const char* reason){skipped_primitives++;render_warning=reason;};
 if(mode&0xb0){skip("Texture/fog/AA not implemented");return;}
 if(!(drawing[0x4e + context]&(1ull<<32))||((test&0x10000)&&((test>>17)&3)>=2)){skip("Z writes or depth comparisons not implemented");return;}
 const auto alpha=drawing[0x42+context];
 if((mode&0x40)&&(((alpha&3)==3)||(((alpha>>2)&3)==3)||(((alpha>>4)&3)==3)||(((alpha>>6)&3)==3))){skip("Reserved ALPHA selector");return;}
 if(drawing[0x22]&3){skip("SCANMSK not implemented");return;}
 if(!bw||(psm!=0&&psm!=1)){skip("Draw needs PSMCT32/24 framebuffer with nonzero FBW");return;}
 int left=int(sc&2047),right=int((sc>>16)&2047),top=int((sc>>32)&2047),bottom=int((sc>>48)&2047);
 if(left>right||top>bottom)return;
 unsigned base=unsigned(frame&511)*8192;uint32_t mask=uint32_t(frame>>32);
 if(psm==1)mask|=0xff000000;
 uint32_t color=vertices[vertex_count-1].color;
 const bool gouraud=mode&8;
 auto pixel=[&](int x,int y,uint32_t shade){
  if(x<left||x>right||y<top||y>bottom)return;
  auto& old=vram[address32(base,bw,unsigned(x),unsigned(y))];
  uint32_t write_mask=mask;const unsigned source_alpha=shade>>24;
  if(test&1){
   const unsigned method=(test>>1)&7,reference=(test>>4)&255;
   const bool pass=method==1||(method==2&&source_alpha<reference)||(method==3&&source_alpha<=reference)||(method==4&&source_alpha==reference)||(method==5&&source_alpha>=reference)||(method==6&&source_alpha>reference)||(method==7&&source_alpha!=reference);
   if(!pass){const unsigned fail=(test>>12)&3;if(fail==0||fail==2)return;if(fail==3)write_mask|=0xff000000;}
  }
  if((test&0x4000)&&psm!=1&&bool(old&0x80000000u)!=bool(test&0x8000))return;
  if((test&0x10000)&&!((test>>17)&3))return;
  if((mode&0x40)&&(!(drawing[0x49]&1)||(shade&0x80000000u))){
   auto color_component=[&](unsigned selector,unsigned shift){return selector==0?int((shade>>shift)&255):selector==1?int((old>>shift)&255):0;};
   const unsigned csel=unsigned((alpha>>4)&3);
   const int factor=csel==0?int(source_alpha):csel==1?int(psm==1?128:old>>24):int((alpha>>32)&255);
   uint32_t output=shade&0xff000000;
   for(unsigned shift=0;shift<24;shift+=8){
    const int product=(color_component(unsigned(alpha&3),shift)-color_component(unsigned((alpha>>2)&3),shift))*factor;
    const int rounded=product>=0?product/128:-((-product+127)/128);
    int value=rounded+color_component(unsigned((alpha>>6)&3),shift);
    if(drawing[0x46]&1)value=std::clamp(value,0,255);
    output|=uint32_t(value&255)<<shift;
   }
   shade=output;
  }
  if(drawing[0x4a+context]&1)shade|=0x80000000;
  old=(old&write_mask)|(shade&~write_mask);pixels++;
 };
 auto floor16=[](int n){return n>=0?n/16:-((-n+15)/16);};
 auto ceil16=[&](int n){return -floor16(-n);};
 if(type==0)pixel(floor16(vertices[0].x),floor16(vertices[0].y),color);
 else if(type==1||type==2){
  int x=floor16(vertices[0].x),y=floor16(vertices[0].y),endx=floor16(vertices[1].x),endy=floor16(vertices[1].y);
  int dx=std::abs(endx-x),dy=-std::abs(endy-y),sx=x<endx?1:-1,sy=y<endy?1:-1,err=dx+dy;
  const int steps=std::max(dx,-dy);int progress=0;
  for(;;){uint32_t shade=color;if(gouraud&&steps){shade=0;for(unsigned ch=0;ch<4;ch++)shade|=uint32_t(((steps-progress)*int((vertices[0].color>>(ch*8))&255)+progress*int((vertices[1].color>>(ch*8))&255))/steps)<<(ch*8);}pixel(x,y,shade);progress++;if(x==endx&&y==endy)break;int e=err*2;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}
 }else if(type==6){
  int x0=std::max(left,ceil16(std::min(vertices[0].x,vertices[1].x))),x1=std::min(right,ceil16(std::max(vertices[0].x,vertices[1].x))-1);
  int y0=std::max(top,ceil16(std::min(vertices[0].y,vertices[1].y))),y1=std::min(bottom,ceil16(std::max(vertices[0].y,vertices[1].y))-1);
  for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++)pixel(x,y,color);
 }else{
  auto a=vertices[0],b=vertices[1],c=vertices[2];
  auto edge=[](Vertex u,Vertex w,int x,int y){return int64_t(w.x-u.x)*(y-u.y)-int64_t(w.y-u.y)*(x-u.x);};
  int64_t area=edge(a,b,c.x,c.y);if(!area)return;if(area<0){std::swap(b,c);area=-area;}
  auto inclusive=[](Vertex u,Vertex w){return w.y<u.y||(w.y==u.y&&w.x>u.x);};
  int x0=std::max(left,floor16(std::min({a.x,b.x,c.x}))),x1=std::min(right,ceil16(std::max({a.x,b.x,c.x}))-1);
  int y0=std::max(top,floor16(std::min({a.y,b.y,c.y}))),y1=std::min(bottom,ceil16(std::max({a.y,b.y,c.y}))-1);
  for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
   auto ab=edge(a,b,x*16+8,y*16+8),bc=edge(b,c,x*16+8,y*16+8),ca=edge(c,a,x*16+8,y*16+8);
   if((ab>0||(ab==0&&inclusive(a,b)))&&(bc>0||(bc==0&&inclusive(b,c)))&&(ca>0||(ca==0&&inclusive(c,a)))){
    uint32_t shade=color;if(gouraud){shade=0;for(unsigned ch=0;ch<4;ch++)shade|=uint32_t((bc*((a.color>>(ch*8))&255)+ca*((b.color>>(ch*8))&255)+ab*((c.color>>(ch*8))&255))/area)<<(ch*8);}
    pixel(x,y,shade);
   }
  }
 }
 primitives++;
}
Gs::Display Gs::scanout()const{
 Display d;int circuit=(regs[0]&1)?0:((regs[0]&2)?1:-1);
 auto rgb=[](uint32_t c){return 0xff000000u|((c&255)<<16)|(c&0xff00)|((c>>16)&255);};
 uint32_t bg=rgb(uint32_t(regs[14]));
 if(circuit<0){d.bgra.assign(d.width*d.height,bg);return d;}
 d.enabled=true;auto fb=regs[7+circuit*2],display=regs[8+circuit*2];
 unsigned bw=unsigned((fb>>9)&63),psm=unsigned((fb>>15)&31),base=unsigned(fb&511)*8192;
 unsigned w=unsigned(((display>>32)&4095)+1)/unsigned(((display>>23)&15)+1),h=unsigned(((display>>44)&2047)+1)/unsigned(((display>>27)&3)+1);
 d.width=std::clamp(w,1u,1024u);d.height=std::clamp(h,1u,1024u);d.bgra.assign(d.width*d.height,bg);
 d.supported=bw&&(psm==0||psm==1);if(!d.supported)return d;
 unsigned startx=unsigned((fb>>32)&2047),starty=unsigned((fb>>43)&2047);
 for(unsigned y=0;y<d.height;y++)for(unsigned x=0;x<d.width;x++)d.bgra[y*d.width+x]=rgb(vram[address32(base,bw,startx+x,starty+y)]);
 return d;
}
}
