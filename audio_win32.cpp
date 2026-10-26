#include "audio_win32.hpp"
#include <chrono>
#include <avrt.h>
namespace ps1 {
bool AudioOutput::open(Spu& spu,std::mutex& machine){
 close();result=MMSYSERR_NOERROR;starvations=host_drops=0;buffered=0;playback_ratio=1;mmcss=false;quit=false;
 event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
 if(!event){result=MMSYSERR_ERROR;return false;}
 WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=2;format.nSamplesPerSec=44100;
 format.wBitsPerSample=16;format.nBlockAlign=4;format.nAvgBytesPerSec=176400;
 result=waveOutOpen(&device,WAVE_MAPPER,&format,reinterpret_cast<DWORD_PTR>(event),0,CALLBACK_EVENT);
 if(result!=MMSYSERR_NOERROR){device=nullptr;close();return false;}
 for(auto& block:blocks){
  block.header={};block.header.lpData=reinterpret_cast<LPSTR>(block.samples.data());block.header.dwBufferLength=DWORD(block.samples.size()*sizeof(int16_t));
  result=waveOutPrepareHeader(device,&block.header,sizeof(WAVEHDR));
  if(result!=MMSYSERR_NOERROR){close();return false;}block.prepared=true;
 }
 // Queue the initial PCM while paused; do not start the device on a single block.
 result=waveOutPause(device);
 if(result!=MMSYSERR_NOERROR){close();return false;}
 ResetEvent(event);
 try{worker=std::thread([this,&spu,&machine]{try{run(spu,machine);}catch(...){result=MMSYSERR_ERROR;}});}
 catch(...){result=MMSYSERR_ERROR;close();return false;}
 return true;
}
void AudioOutput::close(){
 quit=true;if(event)SetEvent(event);if(worker.joinable())worker.join();
 // Join before resetting/unpreparing: the worker must never touch returned headers again.
 if(device){
  waveOutReset(device);
  for(auto& block:blocks){if(block.prepared)waveOutUnprepareHeader(device,&block.header,sizeof(WAVEHDR));block.prepared=false;}
  waveOutClose(device);device=nullptr;
 }
 if(event){CloseHandle(event);event=nullptr;}
}
void AudioOutput::run(Spu& spu,std::mutex& machine){
 using Clock=std::chrono::steady_clock;const auto start=Clock::now();
 // MMCSS registration is scoped to this worker and reverted on every return/exception.
 DWORD task_index=0;
 struct MultimediaTask{HANDLE handle;std::atomic<bool>& enabled;~MultimediaTask(){if(handle)AvRevertMmThreadCharacteristics(handle);enabled=false;}};
 MultimediaTask task{AvSetMmThreadCharacteristicsW(L"Audio",&task_index),mmcss};
 mmcss=task.handle!=nullptr; // If registration is unavailable, continue with normal scheduling.
 AudioPlayback playback;bool primed=false,device_paused=true;
 while(!quit){
  // A 4ms timeout also checks newly generated PCM before the first hardware buffer exists.
  WaitForSingleObject(event,4);ResetEvent(event);if(quit)break;
  std::deque<int16_t> incoming;uint64_t generated=0;
  {std::lock_guard<std::mutex> lock(machine);incoming.swap(spu.audio);generated=spu.generated_frames;}
  playback.append(incoming); // copying, time stretching and device writes happen outside the core lock
  unsigned queued=0;for(const auto& block:blocks)if(block.header.dwFlags&WHDR_INQUEUE)queued+=Frames;
  playback.observe(std::chrono::duration<double>(Clock::now()-start).count(),generated,queued);
  playback_ratio=playback.ratio;host_drops=playback.dropped;
  if(primed&&!queued){starvations++;primed=false;playback.restart();}
  if(!primed){if(!playback.preroll()){buffered=unsigned(playback.input.size()/2);continue;}primed=true;playback.fade_in();}
  for(auto& block:blocks){
   if(block.header.dwFlags&WHDR_INQUEUE)continue;
   if(!playback.render(block.samples.data(),Frames,muted.load())){
    if(queued<=Frames){
     // One fade-to-zero tail before the hardware runs dry, then rebuild a real PCM preroll.
     playback.conceal(block.samples.data(),Frames);
     const auto code=waveOutWrite(device,&block.header,sizeof(WAVEHDR));
     if(code!=MMSYSERR_NOERROR){result=code;return;}
     queued+=Frames;starvations++;primed=false;
    }
    break;
   }
   const auto code=waveOutWrite(device,&block.header,sizeof(WAVEHDR));
   if(code!=MMSYSERR_NOERROR){result=code;return;}
   queued+=Frames;
  }
  if(device_paused&&queued){const auto code=waveOutRestart(device);if(code!=MMSYSERR_NOERROR){result=code;return;}device_paused=false;}
  buffered=unsigned(playback.input.size()/2)+queued;
 }
}
}
