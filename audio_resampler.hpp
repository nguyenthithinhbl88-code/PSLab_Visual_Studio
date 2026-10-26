#pragma once
#include <cstdint>
#include <deque>
namespace ps1 {
// Host playback only: emulated SPU registers, clocks and PCM stay unchanged.
struct AudioResampler {
 double fraction=0;
 bool fill(std::deque<int16_t>& input,int16_t* output,unsigned frames,double ratio,bool muted=false);
};
}
