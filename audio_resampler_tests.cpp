#include "audio_resampler.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 ps1::AudioResampler r;std::deque<int16_t> q;
 for(int n=0;n<1026;n++){q.push_back(int16_t(n*10));q.push_back(int16_t(-n*10));}
 std::array<int16_t,1024> out{};ck(r.fill(q,out.data(),512,0.5),"half-speed block");
 ck(out[0]==0&&out[2]==5&&out[3]==-5&&out[1022]==2555,"linear interpolation and stereo phase");
 ck(q.size()==1540,"consume 256 source frames for 512 output frames");
 auto size=q.size();auto fraction=r.fraction;
 ck(!r.fill(q,out.data(),512,2)&&q.size()==size&&r.fraction==fraction,"underrun leaves state atomic");
 ck(!r.fill(q,out.data(),512,NAN),"invalid ratio rejected");
 ck(r.fill(q,out.data(),512,1,true),"muted consumption");
 for(auto v:out)ck(v==0,"mute contains no stale PCM");
 // A source running at half real time used to leave silence gaps in fixed-rate playback.
 ps1::AudioResampler slow;q.clear();for(unsigned i=0;i<4;i++){q.push_back(0);q.push_back(0);}
 for(unsigned block=0;block<1000;block++){
  for(unsigned i=0;i<256;i++){int16_t sample=int16_t(20000*std::sin((block*256+i)*0.01));q.push_back(sample);q.push_back(int16_t(-sample));}
  ck(slow.fill(q,out.data(),512,0.5),"1000 continuous blocks from a half-speed producer");
  ck(q.size()==8,"stable source queue, no drift or overflow");
 }
 std::cout<<"PASS: adaptive playback interpolation/stereo, ratio consumption, mute, atomic underrun and half-speed continuity\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
