#include "ps2_ee.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(int argc,char** argv){try{
 auto e=std::make_unique<ps2::Ee>();e->chain_demo();
 // Authored ROM, contains no Sony BIOS code/data. Copy packets from ROM to RAM,
 // then execute the EE/DMAC/GIF/GS program from the reset vector itself.
 std::vector<uint8_t> rom(4*1024*1024);std::copy_n(e->ram.begin(),0x1200,rom.begin()+0x10000);
 const uint32_t bootstrap[]={0x3c08bfc1,0x24090000,0x240b0120,
  0x790a0000,0x714054a9,0x7d2a0000,0x25080010,0x25290010,0x256bffff,0x1560fff9,0};
 for(unsigned n=0;n<sizeof(bootstrap)/sizeof(bootstrap[0]);n++)for(unsigned lane=0;lane<4;lane++)rom[n*4+lane]=uint8_t(bootstrap[n]>>(lane*8));
 constexpr unsigned prefix=unsigned(sizeof(bootstrap)/sizeof(bootstrap[0]));
 std::copy_n(e->ram.begin()+0x100000,22*4,rom.begin()+prefix*4);
 const uint32_t loop=0x08000000u|((0xbfc00000u+(prefix+20)*4)>>2&0x3ffffffu);
 for(unsigned lane=0;lane<4;lane++)rom[(prefix+20)*4+lane]=uint8_t(loop>>(lane*8));
 if(argc==2){std::ofstream f(argv[1],std::ios::binary);f.write(reinterpret_cast<const char*>(rom.data()),std::streamsize(rom.size()));ck(bool(f),"write authored pipeline ROM");}
 std::fill(e->ram.begin(),e->ram.end(),0);e->load_bios(rom);
 ck(!e->gs.scanout().enabled,"reset begins before EE enables GS output");
 for(unsigned n=0;n<4000;n++)e->step();
 const auto image=e->gs.scanout();
 ck(e->dma_tags==2&&e->dma_quadwords==32&&e->gs.primitives==6,"ROM copy/MMI/CPU-started DMA chain produces six GS primitives");
 ck(image.enabled&&image.supported&&image.width==640&&image.height==448,"EE SD instructions enable and configure GS scanout");
 ck(e->pc==0xbfc00000u+(prefix+20)*4||e->pc==0xbfc00000u+(prefix+21)*4,"BC0 wait exits after DMA and reaches ROM idle loop");
 ck(e->first_fault.empty()&&e->dma[2].qwc==0&&(e->dmac[1]&4),"ROM pipeline completes without unsupported operations");
 std::cout<<"PASS: authored 4 MiB reset ROM -> LQ/POR/SQ -> RAM -> EE GS/MMIO setup -> DMA chain -> GIF -> six primitives/640x448 scanout; NOT a Sony BIOS compatibility test\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
