#include "core.hpp"
#include "keyboard_pad.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 ps1::KeyboardPad k;ck(k.buttons()==0xffff,"released");
 for(unsigned n=0;n<16;n++)if(k.keys[n]){k.event(k.keys[n],true);ck(k.buttons()==uint16_t(0xffff^(1u<<n)),"each PS1 button bit");k.event(k.keys[n],false);}
 ck(!k.event('B',true)&&!k.event(999,true),"unbound keys ignored");
 k.event('Z',true);k.event(0x26,true);k.event('Q',true);k.event('Z',true);ck(k.buttons()==uint16_t(0xffff^0x4410),"held combo and repeat");ck(k.buttons(false)==0xffff,"focus gate releases");
 ps1::Bus b;auto& p=b.sio[0];p.connected=true;p.buttons=k.buttons();b.write16(0x1f80104a,0x1003);
 const uint8_t tx[]={1,0x42,0,0,0};uint8_t rx[5]{};
 for(unsigned n=0;n<5;n++){b.write8(0x1f801040,tx[n]);rx[n]=b.read8(0x1f801040);if(n==1){k.release();p.buttons=k.buttons();}}
 ck(rx[0]==255&&rx[1]==0x41&&rx[2]==0x5a&&rx[3]==0xef&&rx[4]==0xbb,"keyboard combo reaches digital controller and transaction snapshot");
 b.write16(0x1f80104a,1);b.write16(0x1f80104a,0x1003);
 for(unsigned n=0;n<5;n++){b.write8(0x1f801040,tx[n]);rx[n]=b.read8(0x1f801040);}
 ck(rx[3]==255&&rx[4]==255,"next poll sees released keys");
 // Recover missing native KEYDOWN and KEYUP events using a physical snapshot.
 k.sample([](unsigned key){return key=='Z'||key==0x26;},true);
 ck(k.buttons()==uint16_t(0xffff^0x4010),"physical polling recovers missed key-down");
 k.sample([](unsigned){return false;},true);ck(k.buttons()==0xffff,"physical polling recovers missed key-up");
 bool queried=false;k.sample([&](unsigned){queried=true;return true;},false);ck(!queried&&k.buttons()==0xffff,"background never samples input and releases keys");
 const auto completed=p.completed_polls,pressed=p.pressed_polls;
 b.write16(0x1f80104a,1);b.write16(0x1f80104a,0x1003);p.buttons=0xbfff;
 for(auto value:tx){b.write8(0x1f801040,value);b.read8(0x1f801040);}
 ck(p.completed_polls==completed+1&&p.pressed_polls==pressed+1&&p.last_pressed_buttons==0xbfff,"complete poll records button reply");
 p.buttons=0xffff;ck(p.last_pressed_buttons==0xbfff,"button evidence retained after host release/export");
 std::cout<<"PASS: every keyboard mapping, combos/repeat/focus/release physical polling recovery and actual SIO reply diagnostics\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
