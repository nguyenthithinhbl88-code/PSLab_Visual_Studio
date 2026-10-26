#pragma once
#include <cstdint>
namespace ps1 {
struct Timer {
 uint16_t count=0,mode=0x400,target=0;
 unsigned divider=0;
 bool fired=false,sync_started=false;
 uint16_t read(unsigned reg);
 void write(unsigned reg,uint16_t value);
 bool tick(unsigned index,unsigned external_ticks=0,bool blank=false,bool blank_entry=false);
};
}

namespace ps1 {
inline bool Timer::tick(unsigned index,unsigned external_ticks,bool blank,bool blank_entry){
 if(index<2){
  if(mode&1){const unsigned sync=(mode>>1)&3;
   if(blank_entry){if(sync==1||sync==2)count=0;if(sync==3)sync_started=true;}
   if((sync==0&&blank)||(sync==2&&!blank)||(sync==3&&!sync_started))return false;
  }
  if((mode&0x100)&&!external_ticks)return false;
 }
 if(index==2&&(mode&1)&&(((mode>>1)&3)==0||((mode>>1)&3)==3))return false;
 if(index==2&&(mode&0x200)){if(++divider<8)return false;divider=0;}
 count=uint16_t(count+1);
 bool match=count==target,overflow=count==0xffff;
 if(match)mode|=0x800;
 if(overflow)mode|=0x1000;
 if(match&&(mode&8))count=0;
 bool event=(match&&(mode&0x10))||(overflow&&(mode&0x20));
 if(!event||(!(mode&0x40)&&fired))return false;
 fired=true;
 if(mode&0x80){mode^=0x400;return !(mode&0x400);}
 // Pulse represented as a latched interrupt-controller event; no cycle-level pulse.
 mode|=0x400;return true;
}
}
