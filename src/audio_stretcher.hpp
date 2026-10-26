#pragma once
#include <array>
#include <cstdint>
#include <deque>
namespace ps1 {
// Host-only, stereo-linked waveform-similarity overlap-add. No SPU clock changes.
// Analysis hop follows source tempo; synthesis hop stays fixed to preserve pitch.
struct AudioStretcher {
 static constexpr unsigned Hop=512,Window=Hop*2,Search=256;
 std::array<int16_t,Hop*2> tail{},ready{};
 unsigned ready_offset=Hop;
 double position=0;
 bool primed=false;
 unsigned required(unsigned frames,double ratio)const;
 bool fill(std::deque<int16_t>& input,int16_t* output,unsigned frames,double ratio);
 void reset(){*this=AudioStretcher{};}
private:
 void generate(std::deque<int16_t>& input,double ratio);
};
}
