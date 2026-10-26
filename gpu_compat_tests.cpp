#include "gpu.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 auto gp=std::make_unique<ps1::Gpu>();auto& g=*gp;g.reset();
 g.gp1(0x10000007);ck(g.read_gp0()==2&&g.read_gp0()==2,"v2 GPU version read is stable");
 g.gp1(0x10000017);ck(g.read_gp0()==2,"GPU register index mirrors on four bits");
 g.gp0(0xe2001234);g.gp1(0x10000002);ck(g.read_gp0()==0x1234,"texture window query");
 g.gp1(0x1000000a);ck(g.read_gp0()==0x1234,"query A does not incorrectly mirror query 2");
 g.gp1(0x10000008);ck(g.read_gp0()==0,"GPU query 8 returns zero");
 g.gp1(0x50000007);ck(g.read_gp0()==2,"GP1 command field mirrors on six bits");
 g.gp1(0x43000000);ck(!(g.status&(1u<<23)),"mirrored display enable command");
 g.texture_window=0;g.draw_mode=0x900;g.vram[0]=0x1234;ck(g.texel(0,0,0)==0x1234,"normal 1 MiB texture addressing ignores high Y select");
 g.gp1(0x09000001);ck(g.texel(0,0,0)==0,"selecting missing second VRAM chip yields transparent sample");
 g.gp1(0x09000000);ck(g.texel(0,0,0)==0x1234,"GP1 VRAM option reversible");
 g.gp1(0x09000001);g.gp1(0);ck(!g.allow_texture_y512,"GPU reset clears VRAM size option");
 // Check the opaque fast path against independent 8-to-5-bit quantization.
 for(unsigned n=0;n<65536;n++){uint32_t color=n*2654435761u;g.fragment(3,5,color,0,false,false,false,false);const auto expected=uint16_t(((color&255)/8)|((((color>>8)&255)/8)<<5)|((((color>>16)&255)/8)<<10));ck(g.vram[5*1024+3]==expected,"opaque RGB fast path");}
 g.force_mask=true;g.fragment(3,5,255,0,false,false,false,false);ck(g.vram[5*1024+3]==0x801f,"fast path preserves force mask");g.check_mask=true;g.fragment(3,5,0,0,false,false,false,false);ck(g.vram[5*1024+3]==0x801f,"fast path preserves check mask");
 std::cout<<"PASS: retail GPU version/query latching and mirrors, VRAM chip select, opaque color quantization and mask rules\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
