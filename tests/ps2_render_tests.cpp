#include "ps2_ee.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <set>
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint64_t xy(unsigned x,unsigned y){return uint64_t(x*16)|(uint64_t(y*16)<<16);}
void setup(ps2::Gs& g,unsigned ctx=0){g.draw_write(0x1a,1);g.draw_write(0,ctx?0x200:0);g.draw_write(0x4c+ctx,1ull<<16);g.draw_write(0x4e + ctx,1ull<<32);g.draw_write(0x40+ctx,63ull<<16|63ull<<48);g.draw_write(1,0x80402010);}
}
int main(int argc,char** argv){try{
 auto e=std::make_unique<ps2::Ee>();
 // Replay just the three machine words shown in the user's log. No complete BIOS image is distributed.
 const uint32_t replay[]={0x401a7800,0,0x2b410059};e->reset(0xbfc00000);
 for(unsigned i=0;i<3;i++)for(unsigned b=0;b<4;b++)e->bios[i*4+b]=uint8_t(replay[i]>>(8*b));
 for(unsigned i=0;i<3;i++){e->step();}check(e->pc==0xbfc0000c&&e->r[26].low==0x2e20&&e->r[1].low==0&&e->first_fault.empty(),"reported BIOS SLTI stop resolved");
 e->demo();e->r[26].low=~0ull;e->write(0x100000,0x2b410059,4);e->step();check(e->r[1].low==1,"SLTI signed negative operand");
 e->reset();e->r[26].low=0;e->write(0x100000,0x2b41ffff,4);e->step();check(e->r[1].low==0,"SLTI sign-extended negative immediate");
 ps2::Gs g;check(g.vram.size()*4==4*1024*1024,"4 MiB VRAM");
 check(g.address32(0,1,1,0)==1&&g.address32(0,1,2,0)==4&&g.address32(0,1,0,1)==2&&g.address32(0,1,0,2)==16,"PSMCT32 column addresses");
 check(g.address32(0,1,8,0)==64&&g.address32(0,1,0,8)==128&&g.address32(0,1,16,0)==256&&g.address32(0,1,0,16)==512&&g.address32(0,1,32,0)==1024,"PSMCT32 block addresses");
 check(g.address32(0,2,64,0)==2048&&g.address32(0,2,0,32)==4096&&g.address32(0x3fff00,1,8,0)==0,"page stride and VRAM wrap");
 std::set<uint32_t> page;for(unsigned y=0;y<32;y++)for(unsigned x=0;x<64;x++)page.insert(g.address32(0,1,x,y));check(page.size()==2048&&*page.begin()==0&&*page.rbegin()==2047,"page address bijection");
 setup(g);g.draw_write(5,xy(4,5));check(g.vram[g.address32(0,1,4,5)]==0x80402010&&g.pixels==1,"point draw");
 g.draw_write(0,6);g.draw_write(5,xy(10,10));g.draw_write(5,xy(13,12));check(g.pixels==7&&g.vram[g.address32(0,1,12,11)]==0x80402010&&g.vram[g.address32(0,1,13,12)]==0,"sprite bounds");
 g.draw_write(0x4c,(1ull<<16)|(0xffull<<32));g.draw_write(0,0);g.draw_write(1,0x80ffffff);g.draw_write(5,xy(4,5));check(g.vram[g.address32(0,1,4,5)]==0x80ffff10,"FRAME mask");
 g.draw_write(0x4c,(1ull<<16)|(1ull<<24));g.draw_write(1,0x00123456);g.draw_write(5,xy(4,5));check(g.vram[g.address32(0,1,4,5)]==0x80123456,"PSMCT24 retains high byte");
 auto before=g.pixels;g.draw_write(5,xy(64,64));check(g.pixels==before,"scissor clips point");
 g.reset();setup(g,1);g.draw_write(0x19,16ull|(32ull<<32));g.draw_write(5,xy(3,4));check(g.vram[g.address32(0,1,2,2)]==0x80402010,"context 2 offset");
 g.reset();setup(g);g.draw_write(0,6);g.draw_write(13,xy(1,1));g.draw_write(13,xy(5,5));check(g.pixels==0&&g.vertex_count==0,"XYZ3 advances queue without drawing");
 g.draw_write(5,xy(1,1));g.draw_write(5,xy(3,3));check(g.pixels==4,"queue restarts after XYZ3");
 g.draw_write(0,0x10);g.draw_write(5,xy(0,0));check(g.skipped_primitives==1&&!g.render_warning.empty(),"unsupported texture is diagnosed");
 g.reset();setup(g);g.draw_write(0,3);g.draw_write(5,xy(10,10));g.draw_write(5,xy(30,10));g.draw_write(5,xy(10,30));check(g.pixels==190&&g.vram[g.address32(0,1,12,12)]==0x80402010&&g.vram[g.address32(0,1,29,29)]==0,"triangle coverage and outside");
 // 3-pixel host-to-local IMAGE: final quadword padding must not change a fourth pixel.
 g.reset();g.draw_write(0x50,1ull<<48);g.draw_write(0x51,2ull<<32|3ull<<48);g.draw_write(0x52,3ull|1ull<<32);g.draw_write(0x53,0);
 g.gif_push(1ull|(2ull<<58),0);g.gif_push(0x2222222211111111ull,0x4444444433333333ull);
 check(g.transfer_pixels==3&&g.vram[g.address32(0,1,2,3)]==0x11111111&&g.vram[g.address32(0,1,4,3)]==0x33333333&&g.vram[g.address32(0,1,5,3)]==0,"IMAGE transfer/padding");
 g.draw_write(0x52,2ull|1ull<<32);g.draw_write(0x53,0);g.draw_write(0x54,0x66554433221100ffull);check(g.transfer_pixels==5&&g.vram[g.address32(0,1,3,3)]==0x66554433,"HWREG transfer");
 g.write(0x12000000,1,8);g.write(0x12000070,1ull<<9|2ull<<32|3ull<<43,8);g.write(0x12000080,1ull<<32,8);auto image=g.scanout();check(image.width==2&&image.height==1&&image.bgra[0]==0xffff0011&&image.bgra[1]==0xff334455,"display crop and RGB->BGRA");
 g.write(0x12000000,0,8);g.write(0x120000e0,0x332211,8);image=g.scanout();check(!image.enabled&&image.bgra[0]==0xff112233,"disabled display/background");
 e->gpu_demo();for(unsigned i=0;i<1000;i++)e->step();image=e->gs.scanout();check(image.width==640&&image.height==448&&e->gs.primitives==6&&e->gs.skipped_primitives==0,"CPU/GIF GPU demo scanout");
 check(image.bgra[100*640+100]==0xff33a8d9&&image.bgra[300*640+400]==0xff49c961,"demo rectangle colors");
 if(argc==2){std::ofstream f(argv[1],std::ios::binary);f<<"P6\n"<<image.width<<' '<<image.height<<"\n255\n";for(auto color:image.bgra){char rgb[]={char(color>>16),char(color>>8),char(color)};f.write(rgb,3);}check(bool(f),"PPM export");}
 std::cout<<"PASS: BIOS SLTI regression, VRAM swizzle, primitives/masks/clipping/contexts, IMAGE/HWREG, display crop/BGRA and CPU/GIF GPU demo\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
