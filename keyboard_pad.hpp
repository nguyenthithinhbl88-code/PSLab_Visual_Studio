#pragma once
#include <array>
#include <cstdint>
namespace ps1 {
struct KeyboardPad {
 // PS1 order: Select,L3,R3,Start,Up,Right,Down,Left,L2,R2,L1,R1,Triangle,Circle,Cross,Square.
 static constexpr std::array<unsigned,16> defaults={0x10,0,0,0x0d,0x26,0x27,0x28,0x25,'E','R','Q','W','S','X','Z','A'};
 std::array<unsigned,16> keys=defaults;
 bool neutral_opposites=false;
 void preset(bool wasd){release();keys=wasd?std::array<unsigned,16>{0x09,0,0,0x0d,'W','D','S','A','1','3','Q','E','I','K','J','U'}:defaults;}
 bool bind(unsigned button,unsigned key){
  if(button>=16||button==1||button==2||key>=256||key==0x1b||key==0x12)return false;
  if(key)for(unsigned n=0;n<16;n++)if(n!=button&&keys[n]==key)return false;
  release();keys[button]=key;return true;
 }
 std::array<bool,256> held{};
 // Recover held/released keys even when a native key message was missed.
 template<class IsDown> void sample(IsDown is_down,bool focused){
  if(!focused){release();return;}
  for(auto key:keys)if(key)held[key]=is_down(key);
 }
 bool event(unsigned key,bool down){if(key>=held.size())return false;for(auto k:keys)if(k&&key==k){held[key]=down;return true;}return false;}
 void release(){held.fill(false);}
 uint16_t buttons(bool focused=true)const{uint16_t mask=0xffff;if(focused)for(unsigned n=0;n<keys.size();n++)if(keys[n]&&held[keys[n]])mask&=uint16_t(~(1u<<n));if(neutral_opposites){if(!(mask&0x50))mask|=0x50;if(!(mask&0xa0))mask|=0xa0;}return mask;}
};
}
