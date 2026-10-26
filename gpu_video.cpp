#include "gpu.hpp"
#include <algorithm>
namespace ps1 {
void Gpu::update_video_status(){
 const unsigned total=(display[3]&8)?314:263;
 const unsigned start=display[2]?std::min(display[2]&1023,total):16;
 const unsigned end=display[2]?std::min((display[2]>>10)&1023,total):256;
 video_vblank=end<=start||video_line<start||video_line>=end;
 const bool interlaced=display[3]&32;
 // GPUSTAT.13 is the field indicator, always 1 without interlace.
 const bool field_indicator=!interlaced||!video_field;
 // 480i changes line parity per field; 240-line modes change it per scanline.
 const bool odd=!video_vblank&&(((display[3]&36)==36)?video_field:bool(video_line&1));
 status=(status&~0x80002000u)|(field_indicator?0x2000u:0)|(odd?0x80000000u:0);
}
bool Gpu::tick_video(unsigned timer_sources){
 const bool was_hblank=video_hblank,was_vblank=video_vblank;
 timer_dots=0;timer_hblank_rise=false;
 video_fraction+=11;const unsigned elapsed=video_fraction/7;video_dot+=elapsed;video_fraction%=7;
 if(timer_sources&1){
  constexpr unsigned dividers[]={10,8,5,4};const unsigned divider=(display[3]&64)?7:dividers[display[3]&3];
  pixel_fraction+=elapsed;timer_dots=pixel_fraction/divider;pixel_fraction%=divider;
 }
 const unsigned clocks=(display[3]&8)?3406:3413;
 if(video_dot>=clocks){
  video_dot-=clocks;pixel_fraction=0;
  if(++video_line>=((display[3]&8)?314u:263u)){video_line=0;video_frames++;video_field=!video_field;}
  update_video_status();
 }
 if(timer_sources&2){
  const unsigned start=display[1]?std::min(display[1]&4095,clocks):608;
  const unsigned end=display[1]?std::min((display[1]>>12)&4095,clocks):3168;
  video_hblank=end<=start||video_dot<start||video_dot>=end;
  timer_hblank_rise=!was_hblank&&video_hblank;
 }
 const bool entered_blank=!was_vblank&&video_vblank;
 if(entered_blank)vblank_events++;
 return entered_blank;
}
}
