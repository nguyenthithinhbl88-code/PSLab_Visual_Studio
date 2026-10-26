#pragma once
#include <array>
#include "gpu.hpp"
#include "gte.hpp"
#include "mdec.hpp"
#include "spu.hpp"
#include "timers.hpp"
#include "cdrom.hpp"
#include "sio.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace ps1 {
struct Bus {
    bool direct_exe_boot=false; // diagnostic guard only for a directly loaded EXE
    mutable Gpu gpu;
    mutable Mdec mdec;
    mutable std::array<Sio,2> sio{};
    Spu spu;
    mutable Cdrom cd;
    mutable std::array<Timer,3> timers{};
    uint16_t previous_irq_lines=0;
    void sample_irq_lines();
    void tick_timers();
    std::array<uint32_t,9> memory_control{};
    uint32_t ram_control=0,cache_control=0;
    uint16_t irq_status=0,irq_mask=0;
    uint8_t post=0;
    std::string tty;
    void append_tty(char c);
    bool boot_io_read(uint32_t address,uint8_t& value) const;
    bool boot_io_write(uint32_t address,uint8_t value);
    struct DmaChannel {uint32_t address=0,block=0,control=0,remaining=0,mdec_base=0,chop_wait=0;uint64_t chop_bursts=0;bool active=false;};
    std::array<DmaChannel,7> dma{};
    uint32_t dpcr=0x07654321,dicr=0;
    void update_dma_irq();
    void dma_complete(unsigned channel);
    uint64_t dma_words=0;
    struct CdDmaTrace {uint64_t clock=0;uint32_t lba=0,address=0,words=0,checksum=0;std::array<uint32_t,4> sample{};};
    std::array<CdDmaTrace,32> cd_dma_history{};
    uint64_t cd_dma_transfers=0;
    bool dma_read(uint32_t address,uint32_t& value) const;
    bool dma_write(uint32_t address,uint32_t value);
    bool dma_subword_write(uint32_t address,uint32_t value,unsigned width);
    void run_dma(unsigned channel);
    std::vector<uint8_t> ram = std::vector<uint8_t>(2*1024*1024);
    std::array<uint8_t,1024> scratch{};
    std::vector<uint8_t> bios = std::vector<uint8_t>(512*1024);
    uint8_t read8(uint32_t a) const;
    uint16_t read16(uint32_t a) const;
    uint32_t read32(uint32_t a) const;
    void write8(uint32_t a,uint8_t v);
    void write16(uint32_t a,uint16_t v);
    void write32(uint32_t a,uint32_t v);
    void load_bios(const std::string& path);
};
struct Cpu {
    Bus& bus;
    std::array<uint32_t,32> r{};
    uint32_t pc=0xbfc00000, next_pc=0xbfc00004;
    uint32_t hi=0, lo=0;
    std::array<uint32_t,32> cop0{};
    Gte gte;
    bool delay_slot=false;
    uint64_t instructions=0,interrupts=0,reserved_exceptions=0;
    uint32_t last_reserved_pc=0,last_reserved_opcode=0;
    uint64_t bios_calls=0;
    uint32_t last_bios_vector=0,last_bios_function=0,last_bios_return=0;
    std::array<uint32_t,4> last_bios_args{};
    struct Trace {uint32_t pc=0,opcode=0,rs_value=0,rt_value=0,load_reg=0,load_value=0;};
    std::array<Trace,128> trace{};
    struct FaultSnapshot {
        bool valid=false,has_opcode=false,in_delay=false;
        uint32_t code=0,pc=0,opcode=0,status=0,cause=0,epc=0;
        uint64_t instruction=0;
        std::array<uint32_t,32> registers{},gte_data{},gte_control{};
        std::array<Trace,128> trace{};
    } first_fault;
    bool bios_system_error=false;
    std::string failure_summary()const;
    std::string diagnostic() const;
    unsigned pending_reg=0;
    uint32_t pending_value=0;
    explicit Cpu(Bus& b):bus(b){}
    void reset(uint32_t entry=0xbfc00000);
    void step();
};
void load_exe(Bus& bus,Cpu& cpu,const std::vector<uint8_t>& bytes);
void boot_disc(Bus& bus,Cpu& cpu,bool direct);
void boot_bios(Bus& bus,Cpu& cpu,const std::vector<uint8_t>& image);
void install_demo(Bus& bus);
void install_spu_demo(Bus& bus);
}
