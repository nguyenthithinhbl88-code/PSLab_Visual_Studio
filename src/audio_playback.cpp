#include "audio_playback.hpp"
#include <algorithm>
#include <cmath>
namespace ps1 {
void AudioPlayback::append(std::deque<int16_t>& incoming){
 if(input.empty())input.swap(incoming);
 else{input.insert(input.end(),incoming.begin(),incoming.end());incoming.clear();}
 bool trimmed=input.size()>8820;
 while(trimmed&&input.size()>6144){input.pop_front();input.pop_front();dropped++;}
 if(trimmed)restart();
}
void AudioPlayback::observe(double seconds,uint64_t generated,unsigned device_frames){
 if(!measured||seconds<last_time||generated<last_frames){last_time=seconds;last_frames=generated;measured=true;}
 const double elapsed=seconds-last_time;
 if(elapsed>=0.05){
  const double observed=double(generated-last_frames)/(44100*elapsed);
  rate=std::clamp(0.2*rate+0.8*observed,0.25,2.0);last_time=seconds;last_frames=generated;
 }
 // Count already submitted device samples too; filling all buffers must not look like starvation.
 const double queued=double(input.size()/2)+device_frames*rate;
 const double target=4096*rate+AudioStretcher::Window+AudioStretcher::Search;
 const double correction=std::clamp((queued-target)/65536.0,-0.03,0.03);
 ratio=std::clamp(rate+correction,0.25,2.0);
}
bool AudioPlayback::preroll()const{return input.size()/2>=AudioStretcher{}.required(1536,ratio);}
bool AudioPlayback::render(int16_t* output,unsigned frames,bool muted){
 if(!stretcher.fill(input,output,frames,ratio))return false;
 const double target=muted?0:1;
 for(unsigned n=0;n<frames;n++){
  gain+=std::clamp(target-gain,-1.0/128,1.0/128);
  output[n*2]=int16_t(std::lround(output[n*2]*gain));output[n*2+1]=int16_t(std::lround(output[n*2+1]*gain));
 }
 if(frames)last_sample={output[(frames-1)*2],output[(frames-1)*2+1]};
 return true;
}
void AudioPlayback::conceal(int16_t* output,unsigned frames){
 const unsigned fade=std::min(128u,frames);
 for(unsigned n=0;n<frames;n++)for(unsigned ch=0;ch<2;ch++)output[2*n+ch]=n<fade?int16_t(int(last_sample[ch])*int(fade-1-n)/int(fade)):0;
 last_sample={};restart();
}
}
