#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <deque>
namespace ps1 {
struct Spu {
 std::array<uint16_t,256> regs{};
 std::vector<uint16_t> ram=std::vector<uint16_t>(256*1024);
 std::vector<uint16_t> fifo;
 uint32_t cursor=0;
 uint64_t writes=0;
 struct Voice {
  enum Phase {Off,Attack,Decay,Sustain,Release} phase=Off;
  uint32_t address=0,position=0,envelope_counter=0;
  int previous=0,older=0,level=0;
  unsigned flags=0;
  bool decoded=false;
  std::array<int16_t,31> samples{}; // 3 preceding samples plus current ADPCM block
 };
 std::array<Voice,24> voices{};
 struct Volume {int level=0;unsigned counter=0;};
 std::array<Volume,50> volumes{};
 int volume(unsigned slot,uint16_t value);
 std::deque<int16_t> audio; // interleaved stereo; bounded to 100 ms
 uint64_t generated_frames=0,dropped_frames=0;
 unsigned sample_clock=0,capture_position=0;
 bool irq=false;
 uint16_t noise_level=1;
 int noise_clock=0;
 uint32_t reverb_cursor=0;
 bool reverb_half=false;
 int reverb_left=0,reverb_right=0,send_left=0,send_right=0;
 void reverb(int left,int right);
 void tick(); // one approximate CPU clock; one sample per 768 clocks
 void render_sample(int cd_left=0,int cd_right=0);
 void decode(unsigned index);
 void envelope(unsigned index);
 void check_irq(uint32_t address);
 unsigned active_voices()const;
 uint16_t read(unsigned offset)const;
 void write(unsigned offset,uint16_t value);
 void flush();
 uint32_t dma_read();
 void dma_write(uint32_t value);
};
}
