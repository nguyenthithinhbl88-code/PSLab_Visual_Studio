#include "core.hpp"
#include <chrono>
#include <iostream>
int main(){
 ps1::Bus b;ps1::Cpu c(b);c.reset(0x1000);b.gpu.gp1(0x08000027);b.gpu.gp1(0x07040010);
 const uint32_t code[]={0x3c081f80,0x35081814,0x8d090000,0,0x1000fffd,0};
 for(unsigned i=0;i<6;i++)b.write32(0x1000+i*4,code[i]);
 const auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<5000000;i++)c.step();
 const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 std::cout<<5/seconds<<" MIPS | "<<seconds<<" s | "<<b.gpu.video_frames<<" model fields\nSynthetic GPUSTAT poll; not BIOS FPS, not UI/audio-device performance.\n";
}
