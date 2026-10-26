#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void edge(ps1::Bus& b){for(unsigned i=0;i<3000;i++){b.tick_timers();if(b.gpu.timer_hblank_rise)return;}throw std::runtime_error("no HBlank");}
int main(){try{
 ps1::Bus b;b.timers[0].write(4,0x100);for(unsigned i=0;i<100;i++)b.tick_timers();ck(b.timers[0].count==15,"dot clock default divider10");
 b.timers[1].write(8,3);b.timers[1].write(4,0x158);edge(b);ck(b.timers[1].count==1,"Timer1 HBlank count");edge(b);edge(b);ck(b.timers[1].count==0&&(b.irq_status&32),"HBlank target IRQ5");
 ps1::Timer t;t.write(4,1);t.tick(0,0,true,true);ck(t.count==0,"pause in blank");t.tick(0);ck(t.count==1,"count outside blank");
 t.write(4,3);t.count=100;t.tick(0,0,true,true);ck(t.count==1,"reset at blank edge");
 t.write(4,5);t.tick(0);ck(t.count==0,"blank-only gate");t.tick(0,0,true,true);ck(t.count==1,"blank-only runs");
 t.write(4,7);t.tick(0);ck(t.count==0,"wait first blank");t.tick(0,0,true,true);t.tick(0);ck(t.count==2,"then free run");
 // Actual MIPS loop waiting on HBlank counter; no BIOS bytes.
 ps1::Bus m;ps1::Cpu cpu(m);cpu.reset(0x1000);m.timers[1].write(4,0x100);
 const uint32_t code[]={0x3c081f80,0x35081110,0x8d090000,0,0x1120fffd,0,0x240a0001,0xac0a0100,0x08000408,0};
 for(unsigned i=0;i<10;i++)m.write32(0x1000+i*4,code[i]);
 for(unsigned i=0;i<10000&&m.read32(0x100)!=1;i++)cpu.step();
 ck(m.read32(0x100)==1,"MIPS HBlank wait exits");
 std::cout<<"PASS: dot clock, HBlank source/IRQ, sync gates, bounded MIPS timer polling\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
