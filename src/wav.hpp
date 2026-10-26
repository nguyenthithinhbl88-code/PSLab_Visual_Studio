#pragma once
#include <fstream>
#include <deque>
#include <cstdint>
#include <stdexcept>
namespace ps1 {
class WaveWriter {
 std::ofstream file;uint32_t bytes=0;
 void u16(uint16_t n){file.put(char(n));file.put(char(n>>8));}
 void u32(uint32_t n){u16(uint16_t(n));u16(uint16_t(n>>16));}
public:
 explicit WaveWriter(const std::string& path):file(path,std::ios::binary){
  if(!file)throw std::runtime_error("Cannot create WAV output");
  file.write("RIFF",4);u32(36);file.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(44100);u32(176400);u16(4);u16(16);file.write("data",4);u32(0);
 }
 void append(std::deque<int16_t>& samples){
  if(samples.size()&1)throw std::runtime_error("Stereo sample count must be even");
  if(uint64_t(bytes)+samples.size()*2>0xffffffffu-36)throw std::runtime_error("WAV exceeds RIFF limit");
  for(auto sample:samples)u16(uint16_t(sample));
  bytes+=uint32_t(samples.size()*2);samples.clear();
  if(!file)throw std::runtime_error("WAV write failed");
 }
 void finish(){file.seekp(4);u32(bytes+36);file.seekp(40);u32(bytes);file.flush();if(!file)throw std::runtime_error("WAV finalization failed");}
 ~WaveWriter(){if(file.is_open()){try{finish();}catch(...){}}}
};
}
