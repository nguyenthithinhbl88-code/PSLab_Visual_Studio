#pragma once
#include "disc_image.hpp"
#include "xa_audio.hpp"
#include <array>
#include <cstdint>
#include <deque>
#include <vector>
#include <string>
namespace ps1 {
struct Cdrom {
 DiscImage disc;
 XaAudio xa;
 uint32_t sectors=0,lba=0,last_lba=0;
 uint8_t index=0,enable=31,flag=0,mode=0,filter_file=0,filter_channel=0;
 bool busy=false;
 // One tick currently corresponds to one interpreter instruction (approximate CPU clock).
 static constexpr uint32_t command_latency=0xc4e1,init_latency=0x13cce;
 uint32_t response_delay=0,sector_delay=0,irq_delay=0;
 // Minimum source-low interval after ACK; firmware/mechanical completion is still approximate.
 static constexpr uint32_t irq_ack_gap=1000;
 static constexpr uint32_t single_sector_clocks=451584,double_sector_clocks=225792;
 uint32_t sector_interval()const{return (mode&0x80)?double_sector_clocks:single_sector_clocks;}
 // Drive sector storage is independent of the host-facing data FIFO.
 std::vector<uint8_t> sector_buffer;
 bool buffer_valid=false,sector_pending=false;
 void load_data_fifo();
 uint64_t clocks=0;
 bool playing=false,reading=false,requested=false,motor=true,muted=false,header_valid=false;
 std::array<uint8_t,8> last_header{};
 std::array<uint8_t,4> volume{},active_volume{};
 bool xa_muted=false;
 uint8_t sound_coding=0;
 std::array<uint8_t,2352> audio_sector{};
 unsigned audio_position=588,playing_track=0;
 uint32_t audio_start_delay=0;
 uint64_t cdda_frames=0,cdda_sectors=0;
 std::array<int16_t,2> audio_sample();
 // 0=auto from BOOT executable prefix, 1=USA, 2=Europe, 3=Japan, 4=unlicensed.
 unsigned region=0,detected_region=4;
 std::string boot_name;
 std::deque<uint8_t> params,response,data;
 struct Event{uint8_t irq;std::vector<uint8_t> bytes;};
 struct Pending{uint8_t command;std::deque<uint8_t> params;};
 std::deque<Event> events;
 std::deque<Pending> pending;
 uint64_t commands=0,sectors_read=0;
 uint8_t last_command=0,last_error=0;
 struct Trace {const char* event="";uint8_t value=0,index=0,flag=0,enable=0,status=0;uint64_t clock=0;uint32_t lba=0;unsigned size=0;std::array<uint8_t,16> bytes{};};
 std::array<Trace,64> history{};
 uint64_t transactions=0;
 void record(const char* event,uint8_t value,const std::vector<uint8_t>& bytes={});
 void mount(const std::filesystem::path& path);
 void eject();
 void reset_controller();
 std::vector<uint8_t> sector(uint32_t number);
 uint8_t read(unsigned port);
 void write(unsigned port,uint8_t value);
 void command(uint8_t value);
 void execute(uint8_t value,const std::deque<uint8_t>& parameters);
 void tick();
 void reply(uint8_t irq,std::vector<uint8_t> bytes);
 bool irq()const{return (flag&enable&31)!=0;}
 uint8_t status()const{return sectors?uint8_t((motor?2:0)|(reading?32:0)|(playing?128:0)):0x10;}
 std::vector<uint8_t> read_file(const std::string& path,unsigned limit=4*1024*1024);
 std::string boot_path();
 std::vector<uint8_t> boot_exe();
 unsigned effective_region()const{return region?region:detected_region;}
};
}
