#include "gpu.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
using ps1::Gpu;
using Vertex=Gpu::Vertex;
int64_t edge(const Vertex& a,const Vertex& b,const Vertex& p){return int64_t(b.x-a.x)*(p.y-a.y)-int64_t(b.y-a.y)*(p.x-a.x);}
bool inclusive(const Vertex& a,const Vertex& b){return b.y<a.y||(b.y==a.y&&b.x>a.x);}
// Frozen v30.1 reference: recompute all three edge products at every pixel.
void reference_triangle(Gpu& g,Vertex a,Vertex b,Vertex c,uint16_t clut,bool textured,bool raw,bool semi,bool dither){
 int64_t area=edge(a,b,c);if(!area)return;
 if(std::max({a.x,b.x,c.x})-std::min({a.x,b.x,c.x})>=1024||std::max({a.y,b.y,c.y})-std::min({a.y,b.y,c.y})>=512)return;
 if(area<0){std::swap(b,c);area=-area;}
 const int x0=std::max({0,g.left,std::min({a.x,b.x,c.x})}),x1=std::min({1023,g.right,std::max({a.x,b.x,c.x})});
 const int y0=std::max({0,g.top,std::min({a.y,b.y,c.y})}),y1=std::min({511,g.bottom,std::max({a.y,b.y,c.y})});
 const bool flat=(a.color&0xffffff)==(b.color&0xffffff)&&(a.color&0xffffff)==(c.color&0xffffff);
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
  const Vertex p{x,y,0,0,0};const auto w0=edge(b,c,p),w1=edge(c,a,p),w2=edge(a,b,p);
  if(w0<0||w1<0||w2<0||(w0==0&&!inclusive(b,c))||(w1==0&&!inclusive(c,a))||(w2==0&&!inclusive(a,b)))continue;
  uint32_t color=a.color;
  if(!flat){color=0;for(unsigned shift=0;shift<24;shift+=8)color|=uint32_t((w0*((a.color>>shift)&255)+w1*((b.color>>shift)&255)+w2*((c.color>>shift)&255))/area)<<shift;}
  uint16_t sample=0;
  if(textured){const unsigned u=unsigned((w0*a.u+w1*b.u+w2*c.u)/area),v=unsigned((w0*a.v+w1*b.v+w2*c.v)/area);sample=g.texel(u,v,clut);}
  g.fragment(x,y,color,sample,textured,raw,semi,dither);
 }
}
int main(){try{
 auto ap=std::make_unique<Gpu>(),bp=std::make_unique<Gpu>();auto& a=*ap;auto& b=*bp;
 std::mt19937 rng(0x50533131);unsigned cases=0;std::vector<uint16_t> background(a.vram.size());for(auto& pixel:background)pixel=uint16_t(rng());
 for(unsigned test=0;test<1200;test++){
  a.vram=background;
  a.left=int(rng()%20);a.top=int(rng()%20);a.right=45+int(rng()%40);a.bottom=45+int(rng()%40);
  a.draw_mode=rng()&0x3ff;a.texture_window=rng()&0xfffff;a.force_mask=test&1;a.check_mask=test&2;b=a;
  auto vertex=[&](){return Vertex{int(rng()%140)-30,int(rng()%140)-30,int(rng()%256),int(rng()%256),uint32_t(rng())};};
  auto v0=vertex(),v1=vertex(),v2=vertex();if(test%3==0)v1.color=v2.color=v0.color;
  if(test%17==0)v2=v1;
  const auto clut=uint16_t(rng());const bool tex=test&4,raw=test&8,semi=test&16,dither=test&32;
  a.render_triangle(v0,v1,v2,clut,tex,raw,semi,dither);reference_triangle(b,v0,v1,v2,clut,tex,raw,semi,dither);
  if(a.vram!=b.vram)throw std::runtime_error("incremental raster differs from v30.1 reference at case "+std::to_string(test));
  cases++;
 }
 std::cout<<"PASS: "<<cases<<" seeded full-VRAM raster comparisons, winding/degenerate/clipping, flat/Gouraud, texture formats/window, raw/blended/dithered, masks\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
