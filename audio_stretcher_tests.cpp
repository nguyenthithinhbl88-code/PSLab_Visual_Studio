#include "audio_playback.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <limits>
namespace {
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
std::deque<int16_t> tone(unsigned frames,double hz){std::deque<int16_t> input;for(unsigned i=0;i<frames;i++){auto sample=int16_t(std::lround(12000*std::sin(i*6.283185307179586*hz/44100)));input.push_back(sample);input.push_back(int16_t(-sample));}return input;}
}
int main(){try{
 // Time scaling must preserve frequency, unlike simply resampling at source speed.
 for(double ratio:{0.5,0.75,1.0,1.5,2.0})for(double hz:{110.0,440.0,1000.0}){
  ps1::AudioStretcher stretch;auto input=tone(100000,hz);std::vector<int16_t> output(44100*2);check(stretch.fill(input,output.data(),44100,ratio),"tone stretching succeeds");
  unsigned crossings=0;double energy=0;for(unsigned i=4097;i<44100;i++){if(output[2*(i-1)]<=0&&output[2*i]>0)crossings++;energy+=double(output[2*i])*output[2*i];check(int(output[2*i])+output[2*i+1]==0,"channels share one phase alignment");}
  double frequency=crossings*44100.0/(44100-4097);
  if(std::abs(frequency-hz)>hz*0.025){std::cerr<<"ratio="<<ratio<<" tone="<<hz<<" measured="<<frequency<<'\n';throw std::runtime_error("time stretch changed pitch");}
  check(std::sqrt(energy/(44100-4097))>6500,"tone energy retained");
  double used=(100000-input.size()/2)/44100.0;check(std::abs(used-ratio)<0.04,"source consumption follows tempo");
 }
 // At unity speed, arbitrary stereo content must pass through bit-for-bit.
 ps1::AudioStretcher s;std::deque<int16_t> noise;uint32_t rng=1;for(unsigned i=0;i<12000;i++){rng=rng*1664525+1013904223;noise.push_back(int16_t(rng>>16));}auto original=noise;std::vector<int16_t> out(8192);check(s.fill(noise,out.data(),4096,1),"unity render");for(unsigned i=0;i<8192;i++)check(out[i]==original[i],"unity is sample exact");
 // Insufficient input and invalid ratios must not consume source or change tail/staging.
 auto copy=s;auto before=noise;check(!s.fill(noise,out.data(),4000,2)&&noise==before&&s.position==copy.position&&s.tail==copy.tail&&s.ready_offset==copy.ready_offset,"underrun atomicity");
 check(!s.fill(noise,out.data(),512,std::numeric_limits<double>::quiet_NaN())&&noise==before,"NaN rejected");
 // Non-block-aligned requests carry staging across calls with no gaps.
 s.reset();noise=original;unsigned offset=0;for(unsigned n:{1u,127u,511u,257u,1000u}){check(s.fill(noise,out.data()+offset*2,n,1),"partial packet");offset+=n;}for(unsigned i=0;i<offset*2;i++)check(out[i]==original[i],"partial packet continuity");
 ps1::AudioPlayback p;auto input=tone(4096,440);p.append(input);check(p.render(out.data(),512,false),"playback render");auto final=p.last_sample;p.conceal(out.data(),512);check(std::abs(int(out[0])-final[0])<200&&out[254]==0&&out[1023]==0&&!p.stretcher.primed&&p.gain==0,"underrun fades to silence and resets overlap");
 p=ps1::AudioPlayback{};input=tone(4096,440);p.append(input);p.render(out.data(),512,false);input=tone(10000,110);p.append(input);check(p.dropped>0&&!p.stretcher.primed&&p.gain==0,"queue trim discards stale overlap and fades back in");
 // Producer jitter and speed changes, using 4ms host callbacks over 12 seconds.
 p=ps1::AudioPlayback{};uint64_t generated=0;double fraction=0,device_fraction=0;unsigned queued=0,underruns=0,submitted=0;bool primed=false;unsigned held=0;
 for(unsigned ms=0;ms<12000;ms+=4){
  double speed=ms<4000?0.5:ms<8000?1.0:0.75;fraction+=44100*speed*0.004;unsigned count=unsigned(fraction);fraction-=count;
  // Hold a short burst every 200 ms, then release it four milliseconds later.
  if(ms%200==0){held=count;count=0;}else if(held){count+=held;held=0;}
  generated+=count;input.assign(count*2,5000);p.append(input);
  device_fraction+=44100*0.004;unsigned done=unsigned(device_fraction);device_fraction-=done;queued=queued>done?queued-done:0;
  p.observe(ms/1000.0,generated,queued);
  if(!primed){if(!p.preroll())continue;primed=true;}
  if(!queued&&ms>1000)underruns++;
  while(queued<3584){if(!p.render(out.data(),512,false))break;queued+=512;submitted+=512;}
 }
 check(underruns<4&&submitted>490000&&p.dropped==0,"sustained jitter/tempo-change playback stays bounded");
 std::cout<<"PASS: WSOLA pitch/energy/tempo at 15 tone-rate pairs, linked stereo, exact unity, atomic short input, partial packets, fade/recovery/trim and 12s jitter simulation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
