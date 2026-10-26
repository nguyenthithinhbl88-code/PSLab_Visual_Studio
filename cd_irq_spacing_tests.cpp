#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 ck(argc==2,"need original CUE fixture");ps1::Bus b;b.cd.mount(argv[1]);
 auto w=[&](unsigned p,uint8_t v){b.write8(0x1f801800+p,v);};
 auto ack=[&](){w(0,1);w(3,7);w(0,0);};
 auto wait=[&](){for(unsigned n=0;n<200000;n++){b.tick_timers();if(b.cd.flag)return;}throw std::runtime_error("CD response deadline");};
 // Replay the ordering in the real Init loop: clear CD, finish handling,
 // then clear I_STAT. A next-tick INT2 would be cleared by the first ISR.
 w(1,0xa);wait();ck(b.cd.flag==3&&(b.irq_status&4),"Init INT3 edge");ack();
 for(unsigned n=0;n<200;n++)b.tick_timers();
 ck(b.cd.flag==0&&b.cd.response.empty(),"Init INT2 must not overwrite first handler response window");
 b.write32(0x1f801070,~4u);
 for(unsigned n=200;n<999;n++)b.tick_timers();
 ck(!b.cd.flag&&!(b.irq_status&4),"IRQ source stays low for full minimum ACK interval");b.tick_timers();
 ck(b.cd.flag==2&&(b.irq_status&4),"Init completion generates a new IRQ2 edge after first ISR ACK");
 ack();for(unsigned n=0;n<1500;n++)b.tick_timers();
 // Actual MIPS handler: two interrupts acknowledged separately, completion marker.
 b.cd.reset_controller();b.irq_status=0;b.previous_irq_lines=0;b.irq_mask=4;
 ps1::Cpu c(b);c.reset(0x1000);c.cop0[12]=0x401;
 const uint32_t main[]={0x3c081f80,0x35081800,0x2409000a,0xa1090001,0x08000404,0};
 for(unsigned n=0;n<6;n++)b.write32(0x1000+n*4,main[n]);
 // k0 saves EPC. k1/t0/t1 are scratch. Read HINTSTS before clearing the source.
 const uint32_t handler[]={0x401a7000,0x3c1b1f80,0x24080001,0xa3681800,0x93691803,0,0x31290007,0x24080007,0xa3681803,0xa3601800};
 for(unsigned n=0;n<10;n++)b.write32(0x80+n*4,handler[n]);
 unsigned at=0xa8;for(unsigned n=0;n<200;n++,at+=4)b.write32(at,0); // finishing work in first ISR
 const uint32_t tail[]={0x2408fffb,0xaf681070,0xac090400,0x03400008,0x42000010};
 for(auto word:tail){b.write32(at,word);at+=4;}
 for(unsigned n=0;n<300000&&b.read32(0x400)!=2;n++)c.step();
 ck(b.read32(0x400)==2&&c.interrupts==2&&b.cd.commands==1,"MIPS Init completes after two separate IRQ entries without retry");
 std::cout<<"PASS: Init first/second response spacing, delayed I_STAT ACK replay, actual MIPS two-IRQ completion\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
