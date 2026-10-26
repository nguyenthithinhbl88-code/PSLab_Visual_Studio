#pragma once
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <deque>
#include "ps2_gs.hpp"
namespace ps2 {
struct Register {uint64_t low=0,high=0;};
struct Ee {
 std::array<Register,32> r{};
 Register hi{},lo{};std::array<uint32_t,32> cop0{};Gs gs;
 // Raw COP1 register bits; arithmetic execution is not implemented.
 std::array<uint32_t,32> fpr{};
 uint32_t fcr0=0x2e00,fcr31=0x01000001;
 void require_cop1()const;bool cop1_transfer(uint32_t instruction);
 std::string cop1_diagnostic()const;
 std::array<uint64_t,32> cache_operations{};
 uint32_t cache_last_address=0;unsigned cache_last_operation=0;
 void cache_maintenance(unsigned operation,uint32_t address);
 std::string cache_diagnostic()const;
 uint32_t sa=0;
 bool execute_mmi(uint32_t instruction);
 std::vector<uint8_t> ram=std::vector<uint8_t>(32*1024*1024),bios=std::vector<uint8_t>(4*1024*1024);
 std::array<uint8_t,16384> scratch{};
 uint32_t pc=0x100000,next=0x100004;uint64_t instructions=0;bool delay=false;
 uint32_t reset_entry=0x100000,mch_ricm=0,mch_drd=0;
 mutable unsigned rdram_device=0;
 // Bootstrap register at F500: conservative storage model only, no DMAC side effects.
 // Experimental MCH backing slots, with explicit command/data handlers.
 std::array<uint32_t,16> mch_registers{};
 mutable uint64_t mch_reads=0;uint64_t mch_writes=0;
 bool mch_read(uint32_t address,uint32_t& value)const;
 bool mch_write(uint32_t address,uint32_t value);
 std::string mch_diagnostic()const;
 uint32_t boot_f500=0;uint64_t boot_f500_writes=0;
 // Narrow IOP page-3 bootstrap latch; not an IOP CPU/peripheral implementation.
 uint8_t iop_boot_3204=0;
 mutable uint64_t iop_boot_3204_reads=0;
 uint64_t iop_boot_3204_writes=0;
 // Narrow page-1 halfword storage; hardware side effects remain unknown.
 uint16_t iop_boot_1472=0;
 mutable uint64_t iop_boot_1472_reads=0;
 uint64_t iop_boot_1472_writes=0;
 // IOP bootstrap/configuration model; no IOP instruction execution.
 uint16_t iop_boot_1470=0;
 static constexpr std::array<uint32_t,21> iop_config_addresses={
  0x1f801000,0x1f801004,0x1f801008,0x1f80100c,0x1f801010,0x1f801014,0x1f801018,0x1f80101c,0x1f801020,
  0x1f801060,0x1f801400,0x1f801404,0x1f801408,0x1f80140c,0x1f801410,0x1f801414,0x1f801418,0x1f80141c,0x1f801420,0x1f801450,0x1f801454};
 std::array<uint32_t,21> iop_config{};
 struct IopDma {uint32_t madr=0,bcr=0,chcr=0,tadr=0;};
 std::array<IopDma,13> iop_dma{};
 std::array<uint32_t,6> iop_dmac{}; // DPCR, DICR, DPCR2, DICR2, DMACEN, DMACINTEN
 mutable uint32_t iop_stat=0,iop_mask=0,iop_ctrl=0;
 mutable uint64_t iop_register_reads=0;
 uint64_t iop_register_writes=0;
 std::vector<uint8_t> iop_ram=std::vector<uint8_t>(2*1024*1024);
 std::array<std::vector<uint8_t>,4> vu_memory={std::vector<uint8_t>(4096),std::vector<uint8_t>(4096),std::vector<uint8_t>(16384),std::vector<uint8_t>(16384)};
 uint8_t* mapped_boot_memory(uint32_t physical);
 const uint8_t* mapped_boot_memory(uint32_t physical)const;
 bool iop_read(uint32_t physical,unsigned bytes,uint32_t& value)const;
 bool iop_write(uint32_t physical,unsigned bytes,uint32_t value);
 void reset_iop_registers();void update_iop_dma_irq();
 std::string iop_diagnostic()const;
 struct Serial {uint32_t lcr=0,ier=0,fcr=0,bgr=0;uint64_t reads=0,writes=0,tx_bytes=0,reset_requests=0;};
 mutable Serial serial{};
 struct Vif {std::array<uint32_t,24> regs{};unsigned payload=0,cursor=0,destination=0,payload_kind=0,memory_at=0;uint64_t packets=0,commands=0;};
 std::array<Vif,2> vif{};
 bool serial_read(uint32_t physical,unsigned bytes,uint32_t& value)const;
 bool serial_write(uint32_t physical,unsigned bytes,uint32_t value);
 bool vif_read(uint32_t physical,uint32_t& value)const;
 bool vif_write(uint32_t physical,uint32_t value);
 void vif_push(unsigned channel,Register value);
 void reset_boot_peripherals();std::string peripheral_diagnostic()const;
 struct TlbEntry {uint32_t mask=0,hi=0,lo0=0,lo1=0;bool written=false;};
 std::array<TlbEntry,48> tlb{};bool random_reloaded=false;
 bool cop0_supported(unsigned reg)const;
 void cop0_write(unsigned reg,uint32_t value);
 bool tlb_operation(uint32_t instruction);
 std::string tlb_diagnostic()const;
 struct Timer {uint16_t count=0,mode=0,compare=0,hold=0;unsigned divider=0;};
 struct Dma {uint32_t chcr=0,madr=0,qwc=0,tadr=0,asr0=0,asr1=0,sadr=0;bool chain_end=false,follow_madr=false;};
 static constexpr std::array<uint32_t,10> dma_bases={0x10008000,0x10009000,0x1000a000,0x1000b000,0x1000b400,0x1000c000,0x1000c400,0x1000c800,0x1000d000,0x1000d400};
 std::array<Timer,4> timers{};std::array<Dma,10> dma{};
 std::array<uint32_t,7> dmac{}; // CTRL, STAT, PCR, SQWC, RBSR, RBOR, STADR
 std::array<uint32_t,6> sif{}; // MSCOM, SMCOM, MSFLAG, SMFLAG, CTRL, BD6
 uint32_t intc_stat=0,intc_mask=0,dma_enable=0x1201,gif_ctrl=0,gif_mode=0;
 unsigned bus_phase=0,video_dot=0,video_line=0;
 uint64_t video_frames=0,hardware_ticks=0,dma_quadwords=0,interrupts=0;
 uint64_t dma_tags=0;
 Register dma_read_quadword(uint32_t address,unsigned channel=2);
 void vif_dma_tick(unsigned channel);
 void gif_dma_tick();void chain_demo();
 bool hblank=false,vblank=false,previous_gs_irq=false;
 bool mmio_read(uint32_t address,uint32_t& value)const;
 bool mmio_write(uint32_t address,uint32_t value);
 void reset_hardware();void hardware_tick();void update_interrupts();bool take_interrupt();
 void dma_demo();std::string hardware_diagnostic()const;
 struct TraceEntry {uint32_t pc=0,word=0;bool delay=false,fetch_fault=false;};
 struct TraceBuffer {
  std::array<TraceEntry,64> items{};unsigned cursor=0,count=0;
  void clear(){cursor=count=0;}
  unsigned size()const{return count;}
  bool empty()const{return count==0;}
  void push_back(TraceEntry value){items[cursor]=value;cursor=(cursor+1)&63;if(count<64)count++;}
  const TraceEntry& at(unsigned i)const{return items[(cursor+64-count+i)&63];}
 };
 std::string console,first_fault;TraceBuffer trace;
 std::string boot_status()const;
 void execute_instruction(uint32_t instruction);
 void reset(uint32_t entry=0x100000);
 void execute();void gif_demo();void gpu_demo();
 void write128(uint32_t address,Register value);
 uint8_t read8(uint32_t address)const;
 void write8(uint32_t address,uint8_t value);
 uint64_t read(uint32_t address,unsigned bytes)const;
 void write(uint32_t address,uint64_t value,unsigned bytes);
 void step();void demo();void register_demo();
 void load_elf(const std::vector<uint8_t>& bytes);
 void load_bios(const std::vector<uint8_t>& bytes);
 std::string diagnostic()const;
};
}
