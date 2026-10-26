#include "timers.hpp"
namespace ps1 {
uint16_t Timer::read(unsigned reg){
 if(reg==0)return count;
 if(reg==8)return target;
 uint16_t value=mode;mode&=~0x1800;return value;
}
void Timer::write(unsigned reg,uint16_t value){
 if(reg==0){count=value;return;}
 if(reg==8){target=value;return;}
 mode=(value&0x3ff)|0x400;count=0;divider=0;fired=false;sync_started=false;
}
}
