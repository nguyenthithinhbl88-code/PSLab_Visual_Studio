#include "core.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
void ck(bool b,const char* m){if(!b)throw std::runtime_error(m);}
struct ReferenceTimer:ps1::Timer {
bool tick(unsigned index,unsigned external_ticks,bool blank,bool blank_entry){
 if(index<2){
  if(mode&1){const unsigned sync=(mode>>1)&3;
   if(blank_entry){if(sync==1||sync==2)count=0;if(sync==3)sync_started=true;}
   if((sync==0&&blank)||(sync==2&&!blank)||(sync==3&&!sync_started))return false;
  }
  if((mode&0x100)&&!external_ticks)return false;
 }
 if(index==2&&(mode&1)&&(((mode>>1)&3)==0||((mode>>1)&3)==3))return false;
 if(index==2&&(mode&0x200)){if(++divider<8)return false;divider=0;}
 count=uint16_t(count+1);
 bool match=count==target,overflow=count==0xffff;
 if(match)mode|=0x800;
 if(overflow)mode|=0x1000;
 if(match&&(mode&8))count=0;
 bool event=(match&&(mode&0x10))||(overflow&&(mode&0x20));
 if(!event||(!(mode&0x40)&&fired))return false;
 fired=true;
 if(mode&0x80){mode^=0x400;return !(mode&0x400);}
 // Pulse represented as a latched interrupt-controller event; no cycle-level pulse.
 mode|=0x400;return true;
}
};
int64_t fd(int64_t a,int64_t b){return a>=0?a/b:-((-a+b-1)/b);}
void reference_transform(std::array<int32_t,64>& values,const std::array<int16_t,64>& scale){
 std::array<int32_t,64> work{};
 for(unsigned pass=0;pass<2;pass++){
  for(unsigned row=0;row<8;row++)for(unsigned col=0;col<8;col++){
   int64_t sum=0;for(unsigned f=0;f<8;f++)sum+=int64_t(values[row+f*8])*fd(scale[col+f*8],8);
   work[col+row*8]=int32_t(fd(sum+4095,8192));
  }values=work;
 }
}
int main(){try{
 uint32_t state=0x12345678;auto random=[&](){state^=state<<13;state^=state>>17;state^=state<<5;return state;};
 ps1::Mdec m;
 for(unsigned trial=0;trial<4000;trial++){
  for(auto& n:m.scale)n=int16_t(random());
  std::array<int32_t,64> a{};for(auto& n:a){const auto v=random();n=trial%3==0?0:trial%3==1?(v%8?0:int(v%2048)-1024):int(v%2048)-1024;}
  auto b=a;reference_transform(a,m.scale);m.transform(b);ck(a==b,"sparse/dense IDCT preserves every output bit");
 }
 for(unsigned index=0;index<3;index++){
  ps1::Timer a;ReferenceTimer b;
  for(unsigned n=0;n<100000;n++){
   if(n%47==0){auto value=uint16_t(random());auto reg=(random()%3)*4;a.write(reg,value);b.write(reg,value);}
   const unsigned external=random()%4;const bool blank=random()&1,entry=random()%9==0;
   ck(a.tick(index,external,blank,entry)==b.tick(index,external,blank,entry),"timer IRQ equivalence");
   ck(a.count==b.count&&a.mode==b.mode&&a.divider==b.divider&&a.fired==b.fired&&a.sync_started==b.sync_started,"timer internal state equivalence");
  }
 }
 ps1::Bus bus;ps1::Cpu cpu(bus);
 for(auto base:{0u,0x80000000u,0xa0000000u})for(auto address:{0x1000u,0x1ffffcu,0x7ffffcu}){
  bus.write32(base|address,0x24021234);cpu.reset(base|address);cpu.step();ck(cpu.r[2]==0x1234&&cpu.trace[0].opcode==0x24021234,"direct RAM fetch aliases/boundary and trace");
 }
 const uint32_t instruction=0x24025678;for(unsigned lane=0;lane<4;lane++)bus.bios[0x7fffc+lane]=uint8_t(instruction>>(8*lane));
 for(auto base:{0x1fc00000u,0x9fc00000u,0xbfc00000u}){cpu.reset(base+0x7fffc);cpu.step();ck(cpu.r[2]==0x5678,"BIOS fetch aliases at final word");}
 cpu.reset(0x1001);cpu.step();ck(((cpu.cop0[13]>>2)&31)==4,"unaligned fetch still raises address exception");
 bool rejected=false;cpu.reset(0x1e000000);try{cpu.step();}catch(const std::runtime_error&){rejected=true;}ck(rejected,"unsupported fetch retains bus failure");
 std::cout<<"PASS: 4000 reference IDCT vectors, 300000 timer state/IRQ steps, fast fetch aliases/boundaries, trace, alignment and bus errors\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
