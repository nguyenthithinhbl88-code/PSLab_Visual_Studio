#include "cdrom.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>
namespace ps1 {
namespace {
uint8_t bcd(unsigned v){return uint8_t((v/10)*16+v%10);}
unsigned dec(uint8_t v){return(v>>4)*10+(v&15);}
bool valid_bcd(uint8_t v){return(v&15)<10&&(v>>4)<10;}
std::array<uint8_t,3> msf(unsigned frame){return {bcd(frame/(60*75)),bcd((frame/75)%60),bcd(frame%75)};}
}
void Cdrom::reset_controller(){
 xa=XaAudio{};
 lba=last_lba=0;index=flag=mode=filter_file=filter_channel=0;enable=31;busy=false;response_delay=sector_delay=irq_delay=0;clocks=0;sector_buffer.clear();buffer_valid=sector_pending=false;playing=reading=requested=muted=header_valid=false;xa_muted=false;sound_coding=0;audio_position=588;audio_start_delay=0;playing_track=0;cdda_frames=cdda_sectors=0;audio_sector.fill(0);active_volume.fill(0);motor=true;
 params.clear();response.clear();data.clear();events.clear();pending.clear();commands=sectors_read=transactions=0;history.fill({});last_command=last_error=0;last_header.fill(0);volume.fill(0);
}
void Cdrom::eject(){disc=DiscImage{};sectors=0;boot_name.clear();detected_region=4;reset_controller();}
void Cdrom::mount(const std::filesystem::path& path){
 Cdrom fresh;fresh.disc.open(path);fresh.sectors=fresh.disc.sectors;
 // GetID region is emulated metadata, not a claim that a BIN contains SCEx wobble.
 try{
  fresh.boot_name=fresh.boot_path();auto name=fresh.boot_name;
  for(char& c:name)c=char(std::toupper(static_cast<unsigned char>(c)));
  const auto slash=name.find_last_of("/\\:");if(slash!=std::string::npos)name.erase(0,slash+1);
  if(name.rfind("SLUS",0)==0||name.rfind("SCUS",0)==0)fresh.detected_region=1;
  else if(name.rfind("SLES",0)==0||name.rfind("SCES",0)==0||name.rfind("SLED",0)==0||name.rfind("SCED",0)==0)fresh.detected_region=2;
  else if(name.rfind("SLPS",0)==0||name.rfind("SLPM",0)==0||name.rfind("SCPS",0)==0)fresh.detected_region=3;
 }catch(const std::exception&){} // Mounting a valid raw disc does not require ISO files.
 *this=std::move(fresh);
}
std::vector<uint8_t> Cdrom::sector(uint32_t n){return disc.user(n);}
void Cdrom::record(const char* event,uint8_t value,const std::vector<uint8_t>& bytes){
 auto& trace=history[transactions++%history.size()];trace={};trace.event=event;trace.value=value;
 trace.clock=clocks;trace.lba=last_lba;trace.index=index;trace.flag=flag;trace.enable=enable;trace.status=status();trace.size=unsigned(std::min(bytes.size(),trace.bytes.size()));
 std::copy_n(bytes.begin(),trace.size,trace.bytes.begin());
}
void Cdrom::reply(uint8_t irq,std::vector<uint8_t> bytes){if(events.size()>=8)throw std::runtime_error("CD response queue full");events.push_back({irq,std::move(bytes)});}
void Cdrom::load_data_fifo(){
 if(!data.empty()||!buffer_valid)return;
 data.assign(sector_buffer.begin(),sector_buffer.end());sector_pending=false;
 record("fifo-load",0);
}
void Cdrom::tick(){
 if(audio_start_delay)audio_start_delay--;
 if(irq_delay)irq_delay--;
 if(reading&&sector_delay)sector_delay--;
 if(requested&&sector_pending&&data.empty())load_data_fifo();
 if(response_delay&&--response_delay)return;
 if(flag||!response.empty()||irq_delay)return;
 if(!events.empty()){busy=false;flag=events.front().irq;for(auto b:events.front().bytes)response.push_back(b);record("response",flag,events.front().bytes);events.pop_front();return;}
 if(!pending.empty()){
  auto command=std::move(pending.front());pending.pop_front();execute(command.command,command.params);return;
 }
 if(reading&&!sector_delay&&data.empty()&&!sector_pending){
  try{
   if(disc.track(lba).mode==DiscImage::Mode::Audio)throw std::runtime_error("CDDA playback not implemented");
   const auto raw=disc.raw(lba);last_lba=lba++;std::copy_n(raw.begin()+12,8,last_header.begin());header_valid=true;
   sectors_read++;sector_delay=sector_interval();
   const bool realtime_audio=raw[15]==2&&(raw[18]&0x44)==0x44;
   const bool matches=raw[16]==filter_file&&raw[17]==filter_channel;
   if(realtime_audio&&((mode&0x40)||(mode&8))){
    if((mode&0x40)&&(!(mode&8)||matches)){xa.decode(raw);record("xa-audio",raw[19]);}
    return; // decoded or filtered XA is never presented as CPU data/INT1
   }
   std::vector<uint8_t> bytes;
   if(mode&32)bytes.assign(raw.begin()+12,raw.end());
   else {const unsigned offset=raw[15]==1?16:24;bytes.assign(raw.begin()+offset,raw.begin()+offset+2048);}
   sector_buffer=std::move(bytes);buffer_valid=sector_pending=true;sector_delay=sector_interval();
   if(requested)load_data_fifo();
   flag=1;response.push_back(status());record("sector",1,std::vector<uint8_t>(sector_buffer.begin(),sector_buffer.begin()+16));
  }catch(const std::exception&){reading=false;last_error=0x80;flag=5;response.push_back(status()|1);response.push_back(last_error);}
 }
}
std::array<int16_t,2> Cdrom::audio_sample(){
 if(!playing){
  if(xa.pcm.empty())return {};
  const auto frame=xa.pcm.front();xa.pcm.pop_front();
  if(muted||xa_muted)return {};
  const int l=(int(frame[0])*active_volume[0]+int(frame[1])*active_volume[3])>>7;
  const int rr=(int(frame[0])*active_volume[1]+int(frame[1])*active_volume[2])>>7;
  return {int16_t(std::clamp(l,-32768,32767)),int16_t(std::clamp(rr,-32768,32767))};
 }
 if(audio_start_delay)return {};
 if(audio_position==588){
  if(lba>=sectors||((mode&2)&&disc.track(lba).number!=playing_track)){
   playing=false;if(lba>=sectors)motor=false;
   reply(4,{status()});record("audio-end",4);return {};
  }
  try{
   const auto& track=disc.track(lba);
   if(track.mode==DiscImage::Mode::Audio)audio_sector=disc.raw(lba);else audio_sector.fill(0);
   last_lba=lba++;audio_position=0;cdda_sectors++;
   // SubQ reports: alternating absolute and relative BCD time at 10-frame intervals.
   if((mode&4)&&last_lba%75%10==0&&events.size()<7){
    const bool relative=((last_lba%75)/10)&1;
    const auto stamp=msf(relative?(last_lba>=track.start?last_lba-track.start:track.start-last_lba):last_lba+150);
    reply(1,{status(),bcd(track.number),uint8_t(last_lba>=track.start?1:0),stamp[0],uint8_t(stamp[1]|(relative?0x80:0)),stamp[2],0,0});
   }
  }catch(const std::exception&){playing=false;last_error=0x80;reply(5,{uint8_t(status()|1),last_error});return {};}
 }
 const auto at=audio_position++*4;cdda_frames++;
 const int l=int16_t(uint16_t(audio_sector[at])|(uint16_t(audio_sector[at+1])<<8));
 const int rr=int16_t(uint16_t(audio_sector[at+2])|(uint16_t(audio_sector[at+3])<<8));
 if(muted)return {};
 const int left=(l*active_volume[0]+rr*active_volume[3])>>7;
 const int right=(l*active_volume[1]+rr*active_volume[2])>>7;
 return {int16_t(std::clamp(left,-32768,32767)),int16_t(std::clamp(right,-32768,32767))};
}
uint8_t Cdrom::read(unsigned p){
 if(p==0)return uint8_t(index|(params.empty()?8:0)|(params.size()<16?16:0)|(!response.empty()?32:0)|((requested&&!data.empty())?64:0)|((busy||!pending.empty())?128:0));
 if(p==1){if(response.empty())return 0;auto v=response.front();response.pop_front();record("result",v);return v;}
 if(p==2){if(!requested||data.empty())return 0;auto v=data.front();data.pop_front();return v;}
 return uint8_t(0xe0|((index&1)?flag:enable));
}
void Cdrom::write(unsigned p,uint8_t v){
 if(p==0){index=v&3;return;}
 if(p==1&&index==0){command(v);return;}
 if(p==2&&index==0){if(params.size()>=16)throw std::runtime_error("CD parameter FIFO full");params.push_back(v);return;}
 if(p==2&&index==1){enable=v&31;record("mask",v);return;}
 if(p==3&&index==0){
  record("request",v);requested=v&0x80;
  if(!requested)data.clear();else load_data_fifo();
  return;
 }
 if(p==3&&index==1){
  record("ack",v);const uint8_t previous_flag=flag;flag&=uint8_t(~(v&31));
  // A second response must not raise IRQ2 inside the first response's ISR.
  // Keep the source low long enough for the CPU to clear its edge latch.
  if(previous_flag&&!flag)irq_delay=irq_ack_gap;
  if(v&31)response.clear(); // HC05 ACK drains unread results, allowing the next response/command.
  if(v&0x40)params.clear();
  return;
 }
 if(index==2&&(p==2||p==3)){volume[p-2]=v;return;}
 if(index==3&&(p==1||p==2)){volume[p+1]=v;return;}
 if(index==3&&p==3){xa_muted=v&1;if(v&0x20)active_volume=volume;return;}
 if(index==2&&p==1){sound_coding=v;return;}
 // Sound map XA upload is not decoded yet.
 if(index==1&&p==1)return;
 throw std::runtime_error("Unsupported CD register bank/write");
}
void Cdrom::command(uint8_t v){
 auto p=params;params.clear();busy=true;record("submit",v,std::vector<uint8_t>(p.begin(),p.end()));
 if(flag||!events.empty()||!response.empty()||!pending.empty()){
  if(pending.size()<4)pending.push_back({v,std::move(p)});else last_error=0x20;
  return;
 }
 execute(v,p);
}
void Cdrom::execute(uint8_t v,const std::deque<uint8_t>& p){
 response_delay=(v==0xa||v==0x1e)?init_latency:command_latency;
 commands++;last_command=v;last_error=0;busy=true;record("execute",v,std::vector<uint8_t>(p.begin(),p.end()));
 auto error=[&](uint8_t code){last_error=code;reply(5,{uint8_t(status()|1),code});};
 auto ack=[&](){reply(3,{status()});};
 auto count=[&](unsigned n){if(p.size()==n)return true;error(0x20);return false;};
 auto media=[&](){if(sectors)return true;error(0x80);return false;};
 switch(v){
 case 1:if(count(0))ack();break;
 case 2:{
  if(!count(3))break;
  if(!valid_bcd(p[0])||!valid_bcd(p[1])||!valid_bcd(p[2])||dec(p[1])>=60||dec(p[2])>=75){error(0x10);break;}
  const int address=int((dec(p[0])*60+dec(p[1]))*75+dec(p[2]))-150;
  if(address<0||uint32_t(address)>=sectors){error(0x10);break;}lba=uint32_t(address);ack();break;}
 case 3:{
  if(p.size()>1){error(0x20);break;}if(!media())break;
  if(!p.empty()&&p[0]){
   if(!valid_bcd(p[0])){error(0x10);break;}
   unsigned n=dec(p[0]);
   if(n>disc.tracks.size())n=disc.track(std::min(lba,sectors-1)).number;
   lba=disc.tracks[n-1].start;
  }
  if(lba>=sectors){error(0x80);break;}
  xa=XaAudio{};reading=false;playing=true;motor=true;header_valid=false;audio_position=588;audio_start_delay=response_delay;last_lba=lba;playing_track=disc.track(lba).number;
  sector_delay=0;data.clear();sector_buffer.clear();buffer_valid=sector_pending=false;ack();break;
 }
 case 6:case 0x1b:if(count(0)&&media()){xa=XaAudio{};playing=false;audio_position=588;motor=true;reading=true;data.clear();sector_buffer.clear();buffer_valid=sector_pending=false;sector_delay=response_delay+sector_interval();ack();}break;
 case 7:case 8:case 9:case 0xa:
  if(!count(0))break;
  xa.pcm.clear();playing=false;audio_position=588;reading=false;sector_delay=0;motor=v!=8;
  if(v==8||v==0xa){data.clear();sector_buffer.clear();buffer_valid=sector_pending=false;}
  if(v==0xa){xa=XaAudio{};mode=filter_file=filter_channel=0;header_valid=false;}
  ack();reply(2,{status()});break;
 case 0xb:case 0xc:if(count(0)){muted=v==0xb;ack();}break;
 case 0xd:if(count(2)){filter_file=p[0];filter_channel=p[1];ack();}break;
 case 0xe:if(count(1)){if(p[0]&16){error(0x10);break;}mode=p[0];ack();}break;
 case 0xf:if(count(0))reply(3,{status(),mode,0,filter_file,filter_channel});break;
 case 0x10:if(count(0)){if(!header_valid){error(0x80);break;}reply(3,std::vector<uint8_t>(last_header.begin(),last_header.end()));}break;
 case 0x11:if(count(0)&&media()){
  const auto& t=disc.track((header_valid||playing)?last_lba:std::min(lba,sectors-1));const auto at=(header_valid||playing)?last_lba:std::min(lba,sectors-1);
  const auto relative=msf(at>=t.start?at-t.start:t.start-at),absolute=msf(at+150);
  reply(3,{bcd(t.number),uint8_t(at>=t.start?1:0),relative[0],relative[1],relative[2],absolute[0],absolute[1],absolute[2]});
 }break;
 case 0x12:if(count(1)&&media()){if(p[0]!=1){error(0x10);break;}ack();reply(2,{status()});}break;
 case 0x13:if(count(0)&&media())reply(3,{status(),bcd(disc.tracks.front().number),bcd(disc.tracks.back().number)});break;
 case 0x14:if(count(1)&&media()){
  if(!valid_bcd(p[0])||dec(p[0])>disc.tracks.size()){error(0x10);break;}
  const auto n=dec(p[0]);const auto at=msf((n?disc.tracks[n-1].start:sectors)+150);reply(3,{status(),at[0],at[1]});
 }break;
 case 0x15:case 0x16:if(count(0)&&media()){xa=XaAudio{};playing=false;audio_position=588;reading=false;sector_delay=0;data.clear();sector_buffer.clear();buffer_valid=sector_pending=false;ack();reply(2,{status()});}break;
 case 0x19:if(count(1)){if(p[0]==0x20)reply(3,{0x95,0x07,0x24,0xc1});else error(0x10);}break;
 case 0x1a:if(count(0)){
  ack();const auto r=effective_region();
  if(sectors&&r>=1&&r<=3)reply(2,{status(),0,0x20,0,'S','C','E',uint8_t(r==1?'A':r==2?'E':'I')});
  else reply(5,{uint8_t(status()|8),sectors?uint8_t(0x80):uint8_t(0x40),0,0,0,0,0,0});
 }break;
 case 0x1e:if(count(0)&&media()){ack();reply(2,{status()});}break;
 default:error(0x40);break;
 }
}
}
