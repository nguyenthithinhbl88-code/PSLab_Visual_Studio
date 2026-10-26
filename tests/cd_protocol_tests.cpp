#include "core.hpp"
#include <stdexcept>
#include <iostream>
void ck(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 ck(argc==2,"need CUE fixture");ps1::Bus b;b.cd.mount(argv[1]);
 auto w=[&](unsigned p,uint8_t v){b.write8(0x1f801800+p,v);};auto r=[&](unsigned p){return b.read8(0x1f801800+p);};
 auto tick=[&](){for(unsigned i=0;i<200000;i++){b.tick_timers();if(b.cd.flag||(!b.cd.busy&&b.cd.events.empty()&&b.cd.pending.empty()))return;}throw std::runtime_error("CD timeout");};
 ck(b.cd.enable==31,"IRQ mask enabled at controller reset");
 // BIOS-style poll without first programming HINTMSK: Test version then Nop.
 w(2,0x20);w(1,0x19);ck(r(0)&128,"BUSYSTS asserted during command transport");
 for(unsigned i=0;i<ps1::Cdrom::command_latency-1;i++)b.tick_timers();
 ck((r(0)&128)&&b.cd.flag==0&&b.cd.response.empty()&&!(b.irq_status&4),"no response or IRQ before command latency");b.tick_timers();
 ck(!(r(0)&128)&&b.cd.flag==3&&(b.irq_status&4),"default mask delivers IRQ2 and busy clears on response");
 ck(r(1)==0x95&&b.cd.response.size()==3,"partially consumed firmware response");
 w(0,1);w(3,7);w(0,0);tick();ck(b.cd.response.empty()&&b.cd.flag==0,"ACK drains unread response bytes");
 b.write32(0x1f801070,~4u);w(1,1);tick();ck(b.cd.flag==3&&(b.irq_status&4)&&r(1)==2,"Nop IRQ edge arrives after ACK");
 // Queue a second command before acknowledging the first, with unread results.
 w(1,0x13);ck(r(0)&128,"queued command keeps BUSYSTS set");
 w(0,1);w(3,31);w(0,0);tick();tick();
 ck(b.cd.flag==3&&b.cd.pending.empty()&&b.cd.response.size()==3,"queued GetTN progresses after ACK");
 w(0,1);w(3,31);w(0,0);tick();
 // Explicit masking must still suppress the host IRQ, and enabling it must assert the line.
 w(0,1);w(2,0);w(0,0);b.write32(0x1f801070,~4u);w(1,1);tick();ck(b.cd.flag==3&&!(b.irq_status&4),"explicit zero mask suppresses IRQ2");
 w(0,1);w(2,31);tick();ck(b.irq_status&4,"unmasking a pending response asserts IRQ2");
 w(3,31);w(0,0);tick();
 // Multi-response commands must not deadlock if the first result is ACKed without reading.
 w(1,0x1e);tick();ck(b.cd.flag==3,"ReadTOC first response");w(0,1);w(3,31);w(0,0);tick();ck(b.cd.flag==2&&b.cd.response.size()==1,"ReadTOC second response after unread ACK");
 w(0,1);w(3,31);w(0,0);tick();
 ck(b.cd.transactions>=15&&b.cd.commands==5,"protocol history and command count");
 const auto log=ps1::Cpu(b).diagnostic();ck(log.find("CD protocol history")!=std::string::npos&&log.find("response_fifo=")!=std::string::npos&&log.find("response value=3")!=std::string::npos,"diagnostic retains response data and controller state");
 b.cd.reset_controller();ck(b.cd.enable==31&&!b.cd.busy&&b.cd.response.empty()&&b.cd.transactions==0,"controller reset restores defaults and clears trace");
 // CPU reads on the default 8-bit CD bus must not accidentally consume other ports.
 w(1,1);tick();const auto status=r(0);const auto response_size=b.cd.response.size();
 ck(b.read32(0x1f801800)==uint32_t(status)*0x01010101u&&b.cd.response.size()==response_size,"default LW repeats HSTS without consuming RESULT");
 ck(b.read16(0x1f801800)==uint16_t(status)*0x101u&&b.cd.response.size()==response_size,"default LHU repeats HSTS");
 b.write32(0x1f801018,0x2000);const auto word=b.read32(0x1f801800);ck(((word>>8)&255)==2&&b.cd.response.empty(),"DEV5 auto-increment enables sequential register reads");
 w(0,1);w(3,31);w(0,0);tick();b.write32(0x1f801018,0);
 b.cd.data={0x11,0x22};b.cd.requested=true;ck(b.read16(0x1f801802)==0x2211&&b.cd.data.empty(),"CD data halfword reads two FIFO bytes");
 // A complete command/ACK/IRQ exchange through actual MIPS instructions.
 ps1::Cpu c(b);c.reset(0x1000);b.irq_status=0;
 const uint32_t code[]={0x3c081f80,0x35081800,0x24090001,0xa1090001,0x3c0a1f80,0x354a1070,0x8d4b0000,0,0x316b0004,0x1160fffc,0,0x910c0001,0,0xa1090000,0x240d0007,0xa10d0003,0x240e0001,0xac0e0100,0x08000412,0};
 for(unsigned i=0;i<20;i++)b.write32(0x1000+i*4,code[i]);
 for(unsigned i=0;i<200000&&b.read32(0x100)!=1;i++)c.step();
 ck(b.read32(0x100)==1&&c.r[12]==2&&b.cd.flag==0&&b.cd.response.empty(),"MIPS firmware-style IRQ2 wait/response/ACK sequence completes");
 // Initialization has its own measured first-response latency, and reset cancels transport.
 b.cd.reset_controller();w(1,0x1e);
 for(unsigned i=0;i<ps1::Cdrom::init_latency-1;i++)b.tick_timers();
 ck(b.cd.busy&&!b.cd.flag&&b.cd.response.empty(),"ReadTOC does not respond before initialization latency");
 b.tick_timers();ck(b.cd.flag==3&&!b.cd.busy,"ReadTOC first response at initialization boundary");
 b.cd.reset_controller();w(1,1);for(unsigned i=0;i<17;i++)b.tick_timers();b.cd.reset_controller();
 for(unsigned i=0;i<ps1::Cdrom::command_latency;i++)b.tick_timers();
 ck(!b.cd.flag&&!b.cd.busy&&b.cd.response.empty()&&!b.cd.response_delay,"reset cancels a scheduled response");
 std::cout<<"PASS: CD reset IRQ default, busy lifecycle, unread response ACK drain, queued command, IRQ masking/edges, multi-response and actual MIPS host protocol\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
