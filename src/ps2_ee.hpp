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
 std::vector<uint8_t> ram=std::vector<uint8_t>(32*1024*1024),bios=std::vector<uint8_t>(4*1024*1024);
 std::array<uint8_t,16384> scratch{};
 uint32_t pc=0x100000,next=0x100004;uint64_t instructions=0;bool delay=false;
 uint32_t reset_entry=0x100000,mch_ricm=0,mch_drd=0;
 mutable unsigned rdram_device=0;
 // Bootstrap register at F500: conservative storage model only, no DMAC side effects.
 uint32_t boot_f500=0;uint64_t boot_f500_writes=0;
 struct TlbEntry {uint32_t mask=0,hi=0,lo0=0,lo1=0;bool written=false;};
 std::array<TlbEntry,48> tlb{};bool random_reloaded=false;
 bool cop0_supported(unsigned reg)const;
 void cop0_write(unsigned reg,uint32_t value);
 bool tlb_operation(uint32_t instruction);
 std::string tlb_diagnostic()const;
 struct Timer {uint16_t count=0,mode=0,compare=0,hold=0;unsigned divider=0;};
 struct Dma {uint32_t chcr=0,madr=0,qwc=0,tadr=0,asr0=0,asr1=0,sadr=0;};
 static constexpr std::array<uint32_t,10> dma_bases={0x10008000,0x10009000,0x1000a000,0x1000b000,0x1000b400,0x1000c000,0x1000c400,0x1000c800,0x1000d000,0x1000d400};
 std::array<Timer,4> timers{};std::array<Dma,10> dma{};
 std::array<uint32_t,7> dmac{}; // CTRL, STAT, PCR, SQWC, RBSR, RBOR, STADR
 std::array<uint32_t,6> sif{}; // MSCOM, SMCOM, MSFLAG, SMFLAG, CTRL, BD6
 uint32_t intc_stat=0,intc_mask=0,dma_enable=0x1201,gif_ctrl=0,gif_mode=0;
 unsigned bus_phase=0,video_dot=0,video_line=0;
 uint64_t video_frames=0,hardware_ticks=0,dma_quadwords=0,interrupts=0;
 bool hblank=false,vblank=false,previous_gs_irq=false;
 bool mmio_read(uint32_t address,uint32_t& value)const;
 bool mmio_write(uint32_t address,uint32_t value);
 void reset_hardware();void hardware_tick();void update_interrupts();bool take_interrupt();
 void dma_demo();std::string hardware_diagnostic()const;
 std::string console,first_fault;std::deque<std::string> trace;
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
