#include "audio_playback.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <cmath>
void ck(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(){try{
 ps1::AudioPlayback p;std::deque<int16_t> incoming;
 for(unsigned i=0;i<2048;i++){incoming.push_back(10000);incoming.push_back(-10000);}p.append(incoming);ck(incoming.empty(),"producer queue relinquished");
 std::array<int16_t,1024> block{};ck(p.render(block.data(),512,false),"initial render");
 ck(block[0]>0&&block[0]<100&&block[254]==10000&&block[255]==-10000,"128-frame fade-in, stereo phase preserved");
 ck(p.render(block.data(),512,true),"muted render");ck(block[0]<10000&&block[254]==0&&block[255]==0,"mute ramps to zero without stale playback");
 auto n=p.input.size();p.ratio=2;ck(!p.render(block.data(),1024,false)&&p.input.size()==n,"insufficient PCM does not partially consume");
 p=ps1::AudioPlayback{};p.observe(0,0,0);p.observe(0.1,2205,0);ck(std::abs(p.rate-0.6)<1e-9,"early slow-source estimate adapts before 250ms");
 p.observe(0.2,4410,0);ck(p.rate<0.53,"half-speed convergence");
 const auto empty_ratio=p.ratio;p.observe(0.2,4410,4096);ck(p.ratio>empty_ratio,"already submitted hardware frames affect queue feedback");
 p.observe(0.01,2,0);ck(p.last_frames==2,"restart of emulated counter resets measurement safely");
 incoming.assign(20000,1);p.append(incoming);ck(p.input.size()==6144&&p.dropped==6928,"bounded host queue");
 // Simulate five seconds of half-speed synthesis and callback-driven output, with source jitter.
 p=ps1::AudioPlayback{};uint64_t generated=0;unsigned failures=0,submitted=0,queued=0;double source_fraction=0,device_fraction=0;
 for(unsigned ms=0;ms<5000;ms+=4){
  source_fraction+=44100*0.5*0.004;unsigned count=unsigned(source_fraction);source_fraction-=count;generated+=count;
  for(unsigned i=0;i<count;i++){incoming.push_back(5000);incoming.push_back(-5000);}p.append(incoming);
  device_fraction+=44100*0.004;const unsigned consumed=unsigned(device_fraction);device_fraction-=consumed;queued=queued>consumed?queued-consumed:0;
  p.observe(ms/1000.0,generated,queued);
  if(ms<100&&!p.preroll())continue;
  while(queued<3584){if(!p.render(block.data(),512,false)){if(ms>1000&&queued==0)failures++;break;}queued+=512;submitted+=512;}
 }
 ck(failures==0&&submitted>200000,"continuous callback output at half speed after startup");
 ck(p.dropped==0&&p.ratio>0.47&&p.ratio<0.53,"stable rate and bounded queue with device feedback");
 std::cout<<"PASS: short-lock PCM handoff, fade/mute, atomic underrun, early rate estimate, device queue feedback, restart/bounds and half-speed callback continuity\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
