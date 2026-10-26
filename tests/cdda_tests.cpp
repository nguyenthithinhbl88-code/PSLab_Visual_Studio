#include "core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 const auto dir=std::filesystem::temp_directory_path()/"ps1_lab_cdda_v21";std::filesystem::create_directories(dir);
 {std::ofstream f(dir/"audio.bin",std::ios::binary);for(unsigned sector=0;sector<3;sector++)for(unsigned n=0;n<588;n++){
  const uint16_t l=12000+sector*1000,r=uint16_t(-8000-int(sector)*1000);
  const char bytes[]={char(l),char(l>>8),char(r),char(r>>8)};f.write(bytes,4);
 }}
 std::ofstream(dir/"audio.cue")<<"FILE \"audio.bin\" BINARY\nTRACK 01 AUDIO\nINDEX 01 00:00:00\nTRACK 02 AUDIO\nINDEX 01 00:00:01\n";
 ps1::Bus b;b.cd.mount(dir/"audio.cue");auto& cd=b.cd;
 auto wait=[&](){for(unsigned n=0;n<200000;n++){cd.tick();if(cd.flag)return unsigned(cd.flag);}throw std::runtime_error("response timeout");};
 auto ack=[&](){cd.write(0,1);cd.write(3,31);cd.write(0,0);};
 auto cmd=[&](unsigned op,std::initializer_list<uint8_t> p){cd.write(0,0);for(auto v:p)cd.write(2,v);cd.write(1,uint8_t(op));return wait();};
 auto matrix=[&](unsigned ll,unsigned lr,unsigned rr,unsigned rl,bool apply){cd.write(0,2);cd.write(2,uint8_t(ll));cd.write(3,uint8_t(lr));cd.write(0,3);cd.write(1,uint8_t(rr));cd.write(2,uint8_t(rl));if(apply)cd.write(3,0x20);cd.write(0,0);};
 ck(cmd(3,{1})==3&&cd.playing&&(cd.status()&128),"Play track ACK/status");ack();
 matrix(128,0,128,0,false);auto pcm=cd.audio_sample();ck(pcm[0]==0&&pcm[1]==0,"matrix staged until ADPCTL apply");
 cd.write(0,3);cd.write(3,0x20);cd.write(0,0);pcm=cd.audio_sample();ck(pcm[0]==12000&&pcm[1]==-8000,"signed little-endian stereo PCM");
 matrix(0,128,0,128,true);pcm=cd.audio_sample();ck(pcm[0]==-8000&&pcm[1]==12000,"ATV cross-routing swaps stereo");
 matrix(64,64,64,64,true);pcm=cd.audio_sample();ck(pcm[0]==2000&&pcm[1]==2000,"mono downmix");
 ck(cmd(0xb,{})==3,"Mute ACK");ack();pcm=cd.audio_sample();ck(!pcm[0]&&!pcm[1]&&cd.playing,"mute consumes drive stream");
 ck(cmd(0xc,{})==3,"Demute ACK");ack();pcm=cd.audio_sample();ck(pcm[0]==2000,"demute resumes output");
 cd.write(0,3);cd.write(3,1);cd.write(0,0);pcm=cd.audio_sample();ck(pcm[0]==2000&&!cd.muted&&cd.xa_muted,"ADPMUTE does not mute CDDA");
 ck(cmd(9,{})==3&&!cd.playing,"Pause stops PCM");ack();ck(wait()==2,"Pause completion");ack();
 ck(cmd(0xe,{2})==3,"autopause mode");ack();matrix(128,0,128,0,true);
 ck(cmd(3,{1})==3,"restart track");ack();for(unsigned i=0;i<588;i++)cd.audio_sample();ck(cd.playing&&cd.lba==1,"exact 588 frames per sector");
 pcm=cd.audio_sample();ck(!cd.playing&&pcm[0]==0,"autopause before next track PCM");ck(wait()==4,"end-track INT4");ack();
 ck(cmd(0xe,{0})==3,"continuous mode");ack();ck(cmd(3,{2})==3,"second CUE track");ack();
 pcm=cd.audio_sample();ck(pcm[0]==13000&&pcm[1]==-9000,"track 2 seek and PCM offset");
 for(unsigned i=1;i<1176;i++){cd.audio_sample();}
 cd.audio_sample();ck(!cd.playing&&!cd.motor&&wait()==4,"end-disc INT4 and motor stop");ack();
 ck(cmd(3,{0xfa})==5,"invalid BCD track produces INT5");ack();
 // Bus clocks -> CD samples -> SPU capture + volume + final PCM, with voice enable off.
 ck(cmd(3,{1})==3,"integration Play");ack();b.spu.write(0x180,0x2000);b.spu.write(0x182,0x2000);b.spu.write(0x1b0,0x4000);b.spu.write(0x1b2,0x4000);b.spu.write(0x1aa,1);b.spu.audio.clear();
 for(unsigned i=0;i<768;i++)b.tick_timers();
 ck(b.spu.ram[0]==12000&&int16_t(b.spu.ram[0x400/2])==-8000,"CD captures precede SPU gains");
 ck(b.spu.audio[0]==3000&&b.spu.audio[1]==-2000,"CD independent enable, signed CD gain and main volume");
 cd.reset_controller();ck(!cd.playing&&cd.cdda_frames==0&&cd.active_volume[0]==0,"reset clears PCM/matrix state");
 std::filesystem::remove_all(dir);
 std::cout<<"PASS: original CUE PCM/track seek, ATV apply/cross/mono, mute, CDDA cadence, autopause/INT4, end-disc and CD-to-SPU capture/mix\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
