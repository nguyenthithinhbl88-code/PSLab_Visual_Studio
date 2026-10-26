#include "ps2_gs.hpp"
#include <iostream>
#include <stdexcept>
namespace {void ck(bool b,const char* why){if(!b)throw std::runtime_error(why);}void packet(ps2::Gs& g,unsigned id,uint64_t low,uint64_t high){g.gif_push(1ull|(1ull<<60),id);g.gif_push(low,high);}}
int main(){try{
 ps2::Gs g;packet(g,1,0x2200000011ull,0x4400000033ull);ck(g.drawing[1]==0x3f80000044332211ull,"packed RGBA/Q default");
 // STQ then RGBA in one packet must retain the Q from STQ.
 g.gif_push(1ull|(2ull<<60),0x12);g.gif_push(0x2222222211111111ull,0x40000000);g.gif_push(0x2200000011ull,0x4400000033ull);ck(g.drawing[2]==0x2222222211111111ull&&g.drawing[1]==0x4000000044332211ull,"packed STQ -> RGBA shared Q");
 packet(g,3,0x456700001234ull,~0ull);ck(g.drawing[3]==0x05671234,"packed UV layout/mask");
 packet(g,5,0x003000000020ull,0x12345678);ck(g.drawing[5]==0x1234567800300020ull,"packed XYZ2 layout");
 auto draws=g.primitives;packet(g,5,0x004000000030ull,(1ull<<47)|0x87654321);ck(g.drawing[13]==0x8765432100400030ull&&g.primitives==draws,"ADC redirects to XYZ3 without draw kick");
 packet(g,4,0x003000000020ull,(0xabcdefull<<4)|(0x44ull<<36));ck(g.drawing[4]==0x44abcdef00300020ull,"packed XYZF layout");
 packet(g,10,0,0x88ull<<36);ck(g.drawing[10]==0x8800000000000000ull,"packed FOG layout");
 for(unsigned id:{6u,7u,8u,9u}){packet(g,id,0x12345678,~0ull);ck(g.drawing[id]==0x12345678,"packed direct TEX/CLAMP");}
 bool failed=false;try{g.gif_push(1ull|(1ull<<60),11);}catch(const std::runtime_error&){failed=true;}ck(failed&&g.gif_faulted,"reserved packed descriptor remains explicit fault");
 std::cout<<"PASS: packed RGBA/Q/STQ/UV/XYZ/XYZF/FOG/TEX/CLAMP, ADC no-draw and reserved descriptor\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
