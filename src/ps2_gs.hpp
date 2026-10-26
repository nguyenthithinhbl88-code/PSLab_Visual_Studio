#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace ps2 {
// Experimental untextured software GS, PSMCT32/24 scanout; no video timing.
struct Gs {
 static constexpr std::array<uint32_t,19> addresses={0x12000000,0x12000010,0x12000020,0x12000030,0x12000040,0x12000050,0x12000060,0x12000070,0x12000080,0x12000090,0x120000a0,0x120000b0,0x120000c0,0x120000d0,0x120000e0,0x12001000,0x12001010,0x12001040,0x12001080};
 static constexpr std::array<const char*,19> names={"PMODE","SMODE1","SMODE2","SRFSH","SYNCH1","SYNCH2","SYNCV","DISPFB1","DISPLAY1","DISPFB2","DISPLAY2","EXTBUF","EXTDATA","EXTWRITE","BGCOLOR","CSR","IMR","BUSDIR","SIGLBLID"};
 std::array<uint64_t,19> regs{};uint64_t writes=0,resets=0,events=0;
 std::array<uint64_t,128> drawing{};
 uint64_t gif_tag_low=0,gif_tag_high=0,gif_words=0,gif_packets=0;
 uint32_t gif_remaining=0,gif_cursor=0;unsigned gif_format=0,gif_nreg=0;
 uint32_t gif_q=0x3f800000;
 bool gif_faulted=false;
 static const char* drawing_name(unsigned id);
 void draw_write(unsigned id,uint64_t value);
 void gif_push(uint64_t low,uint64_t high);
 struct Vertex {int x=0,y=0;uint32_t color=0;};
 struct Display {unsigned width=640,height=448;bool enabled=false,supported=true;std::vector<uint32_t> bgra;};
 std::vector<uint32_t> vram=std::vector<uint32_t>(1024*1024);
 std::array<Vertex,3> vertices{};unsigned vertex_count=0;
 uint64_t primitives=0,pixels=0,skipped_primitives=0,transfer_pixels=0;
 std::string render_warning;
 uint32_t transfer_count=0,transfer_total=0,transfer_width=0,transfer_x=0,transfer_y=0,transfer_base=0,transfer_bw=0;
 static uint32_t address32(unsigned base_bytes,unsigned width_units,unsigned x,unsigned y);
 void vertex(uint64_t value,bool kick);
 void rasterize(unsigned type,unsigned context,uint64_t mode);
 void transfer_start(uint64_t direction);
 void transfer_word(uint64_t value);
 Display scanout()const;
 Gs(){reset();}
 void reset();
 uint64_t read(uint32_t address,unsigned bytes)const;
 void write(uint32_t address,uint64_t value,unsigned bytes);
 void event(uint8_t flags){regs[15]|=flags&31;events++;}
 bool irq()const{return (regs[15]&~(regs[16]>>8)&31)!=0;}
 std::string diagnostic()const;
};
}
