#pragma once
#include <cstdint>
namespace ps1 {
// Functional digital pad model: immediate RX, scheduled ACK (not a full shifter).
struct Sio {
 uint16_t mode=0,control=0,baud=0;
 bool rx=false,irq=false,connected=false,ack=false;
 unsigned phase=0;
 uint16_t buttons=0xffff,latched_buttons=0xffff;
 uint8_t data=255;
 unsigned ack_delay=0,ack_width=0;
 uint64_t polls=0,ack_events=0;
 uint64_t completed_polls=0,pressed_polls=0,transfers=0;
 uint16_t last_pressed_buttons=0xffff;
 uint8_t last_tx=255;
 void tick();
 uint8_t read(unsigned offset);
 void write(unsigned offset,uint8_t value);
};
}
