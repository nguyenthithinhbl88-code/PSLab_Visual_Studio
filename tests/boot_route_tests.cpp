#include "core.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
void ck(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
std::vector<uint8_t> make_exe(){
 std::vector<uint8_t> data(0x810);const std::string magic="PS-X EXE";for(unsigned i=0;i<8;i++)data[i]=uint8_t(magic[i]);
 auto word=[&](unsigned n,uint32_t v){for(unsigned i=0;i<4;i++)data[n+i]=uint8_t(v>>(i*8));};
 word(0x10,0x80010000);word(0x18,0x80010000);word(0x1c,16);word(0x30,0x801fff00);
 // addiu t1,zero,2b; addiu t2,zero,a0; jalr ra,t2; nop
 word(0x800,0x2409002b);word(0x804,0x240a00a0);word(0x808,0x0140f809);return data;
}
int main(int argc,char** argv){try{
 ck(argc==2,"need original homebrew CUE fixture");
 ps1::Bus b;ps1::Cpu c(b);auto exe=make_exe();ps1::load_exe(b,c,exe);
 for(unsigned i=0;i<4;i++)c.step();
 ck(c.pc==0xa0&&c.instructions==4,"real MIPS call reaches A0");
 bool rejected=false;try{c.step();}catch(const std::runtime_error& e){std::string message=e.what();rejected=message.find("kernel RAM vector is empty")!=std::string::npos&&message.find("a0(2b)")!=std::string::npos;}
 ck(rejected&&c.pc==0xa0&&c.instructions==4,"stop before running through empty BIOS RAM");
 ck(c.bios_calls==1&&c.last_bios_vector==0xa0&&c.last_bios_function==0x2b&&c.last_bios_return==0x80010010,"BIOS call captured");
 ck(c.diagnostic().find("RAM vectors A0/B0/C0:")!=std::string::npos,"vectors included in saved diagnostic");
 // A custom homebrew-installed A0 handler must still execute normally.
 b.write32(0xa0,0x03e00008);b.write32(0xa4,0);c.step();c.step();ck(c.pc==0x80010010,"custom RAM handler is allowed");
 // Guard only applies to directly loaded EXEs, not normal bare CPU/RAM execution.
 ps1::Bus bare;ps1::Cpu raw(bare);raw.reset(0xa0);raw.step();ck(raw.pc==0xa4,"bare CPU NOP semantics preserved");
 // Failure must not discard media/CPU state; success resets machine and preserves CUE.
 b.cd.mount(argv[1]);b.cd.region=2;const auto sectors=b.cd.sectors;const auto tracks=b.cd.disc.tracks.size();
 b.write32(0x100,0x12345678);c.r[8]=99;const auto old_pc=c.pc;
 bool bad_bios=false;try{ps1::boot_bios(b,c,std::vector<uint8_t>(1));}catch(const std::runtime_error&){bad_bios=true;}
 ck(bad_bios,"bad BIOS accepted");
 ck(b.cd.sectors==sectors&&b.read32(0x100)==0x12345678&&c.pc==old_pc&&c.r[8]==99,"failed BIOS load preserves machine/media");
 std::vector<uint8_t> bios(512*1024);bios[0]=0x12;ps1::boot_bios(b,c,bios);
 ck(!b.direct_exe_boot&&b.bios[0]==0x12&&c.pc==0xbfc00000&&c.instructions==0,"BIOS reset route");
 ck(b.cd.sectors==sectors&&b.cd.disc.tracks.size()==tracks&&b.cd.region==2,"BIOS load retains mounted CUE and region");
 ck(b.cd.sector(16)[1]=='C'&&b.read32(0x100)==0&&c.r[8]==0,"retained file handles work, RAM and CPU reset");
 // Synthetic ROM bytes, not a Sony BIOS: normal BIOS route may install a RAM handler.
 b.write32(0xa0,0x03e00008);b.write32(0xa4,0);c.reset(0xa0);c.r[31]=0x80010000;c.step();c.step();ck(c.pc==0x80010000,"BIOS-route RAM handler executes");
 ps1::boot_disc(b,c,true);ck(b.direct_exe_boot,"direct disc loader tags route");
 ps1::boot_disc(b,c,false);ck(!b.direct_exe_boot&&c.pc==0xbfc00000&&b.cd.sectors==sectors,"BIOS disc restart clears direct route");
 bool unmapped=false;try{b.read8(0xffffa84c);}catch(const std::runtime_error&){unmapped=true;}ck(unmapped,"unmapped bus guard remains strict");
 std::cout<<"PASS: actual MIPS A0(2b) missing-kernel regression, valid handlers, route diagnostics, BIOS validation/reset/media retention, direct/BIOS restart and strict bus faults\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
