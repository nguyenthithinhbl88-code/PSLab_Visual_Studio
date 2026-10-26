#include "core.hpp"
#include <chrono>
#include <iostream>
#include <cmath>
#include <memory>
using Clock=std::chrono::steady_clock;
int main(){
 auto b=std::make_unique<ps1::Bus>();ps1::Cpu c(*b);c.reset(0x1000);b->gpu.gp1(0x08000027);b->gpu.gp1(0x07040010);
 const uint32_t code[]={0x3c081f80,0x35081814,0x8d090000,0,0x1000fffd,0};
 for(unsigned i=0;i<6;i++)b->write32(0x1000+i*4,code[i]);
 for(unsigned mode=0;mode<2;mode++){
  if(mode){ps1::install_spu_demo(*b);b->timers[0].write(4,0x158);b->timers[0].target=320;b->timers[1].write(4,0x107);}
  auto start=Clock::now();for(unsigned n=0;n<10000000;n++)c.step();
  std::cout<<(mode?"cpu_spu_timers":"cpu_poll")<<" "<<10/std::chrono::duration<double>(Clock::now()-start).count()<<" MIPS\n";
 }
 ps1::Mdec m;for(unsigned v=0;v<8;v++)for(unsigned u=0;u<8;u++)m.scale[u+v*8]=int16_t(std::lround((v?1:std::sqrt(0.5))*std::cos((2*u+1)*v*3.141592653589793/16)*32768));
 for(unsigned dense=0;dense<2;dense++){
  std::array<int32_t,64> input{};input[0]=192;for(unsigned i=1;dense&&i<64;i++)input[i]=int(i%21)-10;
  int64_t sum=0;auto start=Clock::now();for(unsigned n=0;n<100000;n++){auto copy=input;m.transform(copy);sum+=copy[n%64];}
  std::cout<<(dense?"mdec_dense":"mdec_dc")<<" "<<100000/std::chrono::duration<double>(Clock::now()-start).count()<<" blocks/s checksum="<<sum<<"\n";
 }
 std::cout<<"Synthetic probe only; not measured GTA FPS or Windows audio performance.\n";
}
