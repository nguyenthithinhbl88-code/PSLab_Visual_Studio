#pragma once
#include <array>
#include <cstdint>
namespace ps1 {
struct KeyboardPad {
 // PS1 order: Select,L3,R3,Start,Up,Right,Down,Left,L2,R2,L1,R1,Triangle,Circle,Cross,Square.
 static constexpr std::array<unsigned,16> keys={0x10,0,0,0x0d,0x26,0x27,0x28,0x25,'E','R','Q','W','S','X','Z','A'};
 std::array<bool,256> held{};
 bool event(unsigned key,bool down){if(key>=held.size())return false;for(auto k:keys)if(k&&key==k){held[key]=down;return true;}return false;}
 void release(){held.fill(false);}
 uint16_t buttons(bool focused=true)const{uint16_t mask=0xffff;if(focused)for(unsigned n=0;n<keys.size();n++)if(keys[n]&&held[keys[n]])mask&=uint16_t(~(1u<<n));return mask;}
};
}
