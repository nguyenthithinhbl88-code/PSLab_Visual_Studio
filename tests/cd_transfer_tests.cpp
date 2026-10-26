#include "core.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 ck(argc==2,"need original CUE fixture");ps1::Bus b;b.cd.mount(argv[1]);
 auto w=[&](unsigned p,uint8_t v){b.write8(0x1f801800+p,v);};
 auto ack=[&](){w(0,1);w(3,31);w(0,0);};
 auto wait=[&](){for(unsigned n=0;n<1000000;n++){b.tick_timers();if(b.cd.flag)return;}throw std::runtime_error("CD transfer timeout");};
 auto command=[&](uint8_t cmd,std::initializer_list<uint8_t> params){for(auto v:params)w(2,v);w(1,cmd);wait();ck(b.cd.flag==3,"command ACK");ack();};
 command(2,{0,2,0x16});command(6,{});wait();ck(b.cd.flag==1,"first data IRQ");
 const auto first=b.cd.sector(16);const auto lba=b.cd.lba;const auto count=b.cd.sectors_read;
 // BIOS-style data request reset must retain the drive's current sector.
 w(3,0);w(3,0x80);ck(b.cd.data.size()==2048,"BFRD reset then request loads current sector");
 for(unsigned n=0;n<8;n++)ck(b.read8(0x1f801802)==first[n],"current sector starts at correct byte after request reset");
 w(3,0);w(3,0x80);for(unsigned n=0;n<8;n++)ck(b.read8(0x1f801802)==first[n],"BFRD re-request restarts same buffered sector");
 ck(b.cd.lba==lba&&b.cd.sectors_read==count,"FIFO reload never advances disc");
 // DMA3 -> RAM round trip with per-byte comparison, including a partial transfer.
 w(3,0);w(3,0x80);b.dpcr=0x8000;
 b.write32(0x1f8010b0,0x10000);b.write32(0x1f8010b4,7);b.write32(0x1f8010b8,0x11000000);
 ck(b.cd.data.size()==2048-28&&!(b.dma[3].control&(1u<<24)),"short DMA consumes only requested bytes");
 for(unsigned n=0;n<28;n++)ck(b.ram[0x10000+n]==first[n],"partial DMA data matches disc");
 w(3,0);w(3,0x80);b.write32(0x1f8010b0,0x11000);b.write32(0x1f8010b4,512);b.write32(0x1f8010b8,0x11000000);
 for(unsigned n=0;n<2048;n++)ck(b.ram[0x11000+n]==first[n],"full DMA data matches disc");
 ack();const auto before=b.cd.clocks;for(unsigned n=0;n<1000;n++)b.tick_timers();ck(!b.cd.flag&&b.cd.sectors_read==count,"sector does not arrive immediately after ACK");wait();
 const auto single_gap=b.cd.clocks-before;ck(single_gap>400000&&single_gap<460000,"single-speed sector interval");
 ack();command(9,{});wait();ack();
 // Double-speed cadence, DMA waiting for the next sector, reset cancellation.
 command(0xe,{0x80});command(2,{0,2,0x16});command(6,{});
 w(3,0x80);b.write32(0x1f8010b0,0x14000);b.write32(0x1f8010b4,1024);b.write32(0x1f8010b8,0x11000000);
 wait();ck(b.cd.flag==1&&b.dma[3].block==512,"DMA waits then consumes first sector");ack();const auto clock=b.cd.clocks;wait();
 ck(b.cd.clocks-clock==225792,"double-speed sector interval");ck(!(b.dma[3].control&(1u<<24)),"two-sector DMA completes");
 const auto second=b.cd.sector(17);for(unsigned n=0;n<2048;n++){ck(b.ram[0x14000+n]==first[n],"multi-DMA first sector");ck(b.ram[0x14800+n]==second[n],"multi-DMA second sector");}
 ck(b.cd_dma_transfers==4&&b.cd_dma_history[0].words==7&&b.cd_dma_history[1].words==512&&b.cd_dma_history[2].lba==16&&b.cd_dma_history[3].lba==17,"DMA history identifies transfer addresses/counts/LBA");
 ck(b.cd_dma_history[0].sample[0]==0x30444301,"DMA trace samples actual PVD bytes");
 b.cd.reset_controller();for(unsigned n=0;n<500000;n++)b.tick_timers();ck(!b.cd.flag&&b.cd.data.empty(),"reset cancels scheduled sectors");
 std::cout<<"PASS: BFRD restart without sector loss, exact DMA bytes, partial/multi-sector DMA, 1x/2x cadence and reset\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
