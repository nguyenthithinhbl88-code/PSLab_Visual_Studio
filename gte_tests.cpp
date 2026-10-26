#include "gte.hpp"
#include <iostream>
#include <stdexcept>
using ps1::Gte;
void ck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint32_t xy(int x,int y){return uint16_t(x)|(uint32_t(uint16_t(y))<<16);}
void identity(Gte& g,unsigned base){g.write_control(base,4096);g.write_control(base+1,0);g.write_control(base+2,4096);g.write_control(base+3,0);g.write_control(base+4,4096);}
void command(Gte& g,uint32_t n){ck(g.execute(n),"documented GTE command rejected");}
int main(){try{
 Gte g;
 g.write_data(1,0x12348000);ck(g.read_data(1)==0xffff8000,"VZ sign extension");
 g.write_data(7,0xffff1234);g.write_data(16,0xffffabcd);ck(g.read_data(7)==0x1234&&g.read_data(16)==0xabcd,"unsigned depth registers");
 g.write_control(26,0xffff);ck(g.read_control(26)==0xffffffff,"H read sign-extension hardware quirk");
 g.write_control(31,0xffffffff);ck(g.read_control(31)==0xfffff000,"FLAG write mask and summary");g.write_control(31,1u<<22);ck(g.read_control(31)==(1u<<22),"IR3 alone does not set summary");
 g.write_data(12,1);g.write_data(13,2);g.write_data(14,3);g.write_data(15,4);ck(g.read_data(12)==2&&g.read_data(13)==3&&g.read_data(14)==4&&g.read_data(15)==4,"SXY push and alias");
 g.write_data(28,0x7fff);ck(g.read_data(9)==3968&&g.read_data(10)==3968&&g.read_data(11)==3968&&g.read_data(29)==0x7fff,"IRGB expansion and ORGB conversion");
 g.write_data(9,0xffff);g.write_data(10,0x7fff);g.write_data(11,0x80);ck(g.read_data(28)==(31u<<5|1u<<10),"ORGB saturation");
 g.write_data(31,123);ck(g.read_data(31)==0,"LZCR read only");
 for(auto value:{0u,0xffffffffu,1u,0x80000000u,0x40000000u,0xfffffffeu}){
  g.write_data(30,value);unsigned expected=(value==0||value==0xffffffffu)?32:(value==1||value==0xfffffffeu)?31:value==0x80000000u?1:1;
  ck(g.read_data(31)==expected,"leading sign-bit count");
 }
 g=Gte{};identity(g,0);g.write_control(5,10);g.write_control(6,uint32_t(-20));g.write_control(7,30);g.write_data(0,xy(100,200));g.write_data(1,uint32_t(-300));
 command(g,0x80012);ck(g.read_data(9)==110&&g.read_data(10)==180&&g.read_data(11)==uint32_t(-270),"MVMVA identity and translation");
 command(g,0x80412);ck(g.read_data(11)==0&&(g.read_control(31)&(1u<<22))&&!(g.read_control(31)&0x80000000u),"MVMVA lm saturation and FLAG summary");
 g.write_data(9,uint32_t(-4));g.write_data(10,uint32_t(-3));g.write_data(11,12);command(g,0x28);ck(g.read_data(9)==16&&g.read_data(10)==9&&g.read_data(11)==144,"SQR signed vector");
 g.write_control(0,1);g.write_control(2,2);g.write_control(4,3);g.write_data(9,4);g.write_data(10,5);g.write_data(11,6);command(g,0x0c);ck(g.read_data(9)==uint32_t(-3)&&g.read_data(10)==6&&g.read_data(11)==uint32_t(-3),"OP cross product");
 g=Gte{};g.write_data(12,xy(0,0));g.write_data(13,xy(10,0));g.write_data(14,xy(0,10));command(g,6);ck(g.read_data(24)==100,"NCLIP orientation area");
 g.write_data(13,xy(0,10));g.write_data(14,xy(10,0));command(g,6);ck(g.read_data(24)==uint32_t(-100),"NCLIP reverse winding");
 for(unsigned i=0;i<4;i++)g.write_data(16+i,100*(i+1));
 g.write_control(29,4096);g.write_control(30,1024);
 command(g,0x2d);ck(g.read_data(7)==900,"AVSZ3 selected depths");command(g,0x2e);ck(g.read_data(7)==250,"AVSZ4 quarter scale");g.write_control(29,0xffff);command(g,0x2d);ck(g.read_data(7)==0&&(g.read_control(31)&(1u<<18)),"negative OTZ saturation");
 g=Gte{};identity(g,0);g.write_control(7,1000);g.write_control(26,256);g.write_control(24,320u<<16);g.write_control(25,240u<<16);g.write_data(0,xy(100,-50));
 command(g,0x180001);ck(g.read_data(9)==100&&g.read_data(10)==uint32_t(-50)&&g.read_data(19)==1000,"RTPS rotation translation and depth");
 ck(g.read_data(14)==xy(345,227)&&g.read_control(31)==0,"RTPS projection golden point");
 g.write_data(2,xy(0,0));g.write_data(4,xy(-100,50));command(g,0x280030);
 ck(g.read_data(12)==xy(345,227)&&g.read_data(13)==xy(320,240)&&g.read_data(14)==xy(294,252),"RTPT projected XY FIFO order");
 g.write_control(7,0);command(g,0x180001);ck((g.read_control(31)&0x80020000)==0x80020000,"RTPS zero-depth divide overflow");
 g.write_control(7,100000);command(g,0x180001);ck(g.read_data(19)==65535&&(g.read_control(31)&(1u<<18)),"far depth clamp");
 // UNR special case documented by PSX-SPX: clamp 0x20000 to 0x1ffff without divide-overflow.
 g=Gte{};identity(g,0);g.write_control(7,0x7f20);g.write_control(26,0xfe3f);command(g,0x180001);ck(!(g.read_control(31)&(1u<<17)),"UNR near-limit clamp is not overflow");
 // RTPT must retain an earlier vertex's IR3 saturation flag.
 g=Gte{};identity(g,0);g.write_data(1,32767);g.write_data(3,1);g.write_data(5,1);g.write_control(7,1);command(g,0x280030);ck(g.read_control(31)&(1u<<22),"RTPT accumulates saturation across vertices");
 g=Gte{};g.write_data(6,0x20000000);g.write_data(8,4096);g.write_data(9,16);g.write_data(10,32);g.write_data(11,48);command(g,0x8003d);ck(g.read_data(22)==0x20030201,"GPF color FIFO");command(g,0x8003e);ck(g.read_data(22)==0x20060402,"GPL MAC base");
 g=Gte{};g.write_data(6,0x20503010);g.write_control(21,128*16);g.write_control(22,64*16);g.write_control(23,32*16);command(g,0x80010);ck(g.read_data(22)==0x20503010,"DPCS zero factor preserves RGB");g.write_data(8,4096);command(g,0x80010);ck(g.read_data(22)==0x20204080,"DPCS full factor reaches far color");
 g.write_data(8,0);g.write_data(9,10*16);g.write_data(10,20*16);g.write_data(11,30*16);command(g,0x80011);ck(g.read_data(22)==0x201e140a,"INTPL zero factor");
 g.write_data(6,0x20204080);g.write_data(9,256);g.write_data(10,512);g.write_data(11,1024);command(g,0x80029);ck(g.read_data(22)==0x20080808,"DCPL modulation");
 g.write_data(20,0x00010203);g.write_data(21,0x00040506);g.write_data(22,0x00070809);command(g,0x8002a);ck(g.read_data(20)==0x20010203&&g.read_data(21)==0x20040506&&g.read_data(22)==0x20070809,"DPCT shifts three input colors");
 g=Gte{};identity(g,8);identity(g,16);g.write_data(0,xy(1024,2048));g.write_data(1,4096);g.write_data(6,0x20808080);command(g,0x8041e);ck(g.read_data(22)==0x20ff8040,"NCS lighting and color clamp");
 command(g,0x8041b);ck(g.read_data(22)==0x20804020,"NCCS color modulation");command(g,0x80413);ck(g.read_data(22)==0x20804020,"NCDS zero depth factor");
 // Translation and matrix products cross the 44-bit accumulator boundaries.
 g=Gte{};identity(g,0);g.write_control(5,0x7fffffff);g.write_data(0,32767);command(g,0x80012);ck(g.read_control(31)&(1u<<30),"positive MAC1 44-bit overflow");
 g=Gte{};identity(g,0);g.write_control(5,0x80000000);g.write_data(0,0xffff);command(g,0x80012);ck(g.read_control(31)&(1u<<27),"negative MAC1 44-bit overflow");
 // Every documented opcode must be functional, not rejected as a reserved CPU instruction.
 const unsigned all[]={1,6,12,16,17,18,19,20,22,27,28,30,32,40,41,42,45,46,48,61,62,63};
 for(auto op:all){command(g,0x80400|op);}ck(!g.execute(0x02),"undocumented command is not silently accepted");
 std::cout<<"PASS: GTE register semantics, FLAG, sign count, FIFO/color conversion, matrix/vector, clipping/depth, UNR projection, interpolation and lighting golden vectors; 22 documented commands\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
