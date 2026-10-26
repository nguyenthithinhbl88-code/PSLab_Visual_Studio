#include "cdrom.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(int argc,char** argv){try{
 ck(argc==2,"need authored CUE fixture");ps1::Cdrom c;c.mount(argv[1]);
 auto finish=[&](){c.events.clear();c.response.clear();c.flag=0;c.busy=false;c.response_delay=c.irq_delay=0;};
 auto execute=[&](uint8_t op,std::deque<uint8_t> p={}){c.execute(op,p);ck(!c.events.empty(),"command has response");};
 c.lba=4;c.last_lba=3;c.reading=true;c.header_valid=true;c.last_header[0]=0xab;
 c.sector_delay=12345;c.data={1,2,3};c.sector_buffer={4,5,6};c.buffer_valid=true;c.xa.pcm.push_back({11,22});
 execute(2,{0,2,0x16});ck(c.lba==4&&c.target_lba==16&&c.setloc_pending&&c.reading,"Setloc only stages a target while reading");
 ck(c.data.size()==3&&c.sector_delay==12345&&c.header_valid&&c.last_header[0]==0xab,"Setloc preserves active stream and last header");finish();
 execute(6);ck(c.lba==16&&!c.setloc_pending&&c.reading&&c.data.empty()&&c.xa.pcm.empty(),"Read consumes staged seek and resets old stream");finish();
 c.data={7,8};c.sector_buffer={9,10};c.buffer_valid=c.sector_pending=true;c.sector_delay=777;c.xa.pcm.push_back({33,44});
 execute(0x1b);ck(c.lba==16&&c.data.size()==2&&c.sector_buffer.size()==2&&c.sector_pending&&c.sector_delay==777&&c.xa.pcm.size()==1,"ReadS during ReadN without Setloc preserves data, XA and cadence");finish();
 execute(9);ck(c.events.size()==2&&(c.events[0].bytes[0]&32)&&!(c.events[1].bytes[0]&32)&&!c.reading,"Pause ACK reports old read status, completion reports stopped status");finish();
 execute(2,{0,2,0x17});finish();execute(0x15);ck(c.lba==17&&c.last_lba==17&&!c.setloc_pending&&!c.reading,"Seek consumes staged target and updates reported head");finish();
 execute(2,{0,2,0x16});finish();execute(3);ck(c.lba==16&&c.playing&&!c.setloc_pending,"Play without track consumes staged target");finish();
 execute(2,{0,2,0x17});finish();execute(0xa);ck(!c.setloc_pending,"Init clears staged seek");finish();
 execute(2,{0,2,0x16});finish();execute(2,{0,0x99,0});ck(c.target_lba==16&&c.setloc_pending&&c.events[0].irq==5,"invalid Setloc preserves previous target");
 std::cout<<"PASS: staged Setloc during streaming, consuming target in Read/Seek/Play, repeated Read preserves FIFO/XA/cadence, Pause status phases and Init/invalid target\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
