#include "core.hpp"
#include <memory>
#include <stdexcept>
namespace ps1 {
void boot_bios(Bus& bus,Cpu& cpu,const std::vector<uint8_t>& image){
 if(image.size()!=512*1024)throw std::runtime_error("BIOS must be exactly 512 KiB");
 auto fresh=std::make_unique<Bus>();fresh->bios=image;
 // Validate and allocate before moving the mounted image/file handles.
 fresh->cd=std::move(bus.cd);fresh->cd.reset_controller();fresh->sio[0].connected=true;
 bus=std::move(*fresh);cpu.reset();
}
void boot_disc(Bus& bus,Cpu& cpu,bool direct){
 if(!bus.cd.sectors)throw std::runtime_error("Mount a disc before booting");
 auto fresh=std::make_unique<Bus>();fresh->bios=bus.bios;
 Cpu next(*fresh);
 if(direct){auto bytes=bus.cd.boot_exe();load_exe(*fresh,next,bytes);}else next.reset();
 const auto entry=next.pc;const auto registers=next.r;
 // Move mounted file handles only after direct-EXE validation succeeds.
 fresh->cd=std::move(bus.cd);fresh->cd.reset_controller();fresh->sio[0].connected=true;
 bus=std::move(*fresh);cpu.reset(entry);cpu.r=registers;
}
}
