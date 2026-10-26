#include "core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
std::array<uint8_t,2352> sector(unsigned coding=1,unsigned packed=0xf1,unsigned header=0){
 std::array<uint8_t,2352> raw{};raw[0]=raw[11]=0;for(unsigned i=1;i<11;i++)raw[i]=255;
 raw[15]=2;raw[16]=1;raw[17]=2;raw[18]=0x64;raw[19]=uint8_t(coding);
 for(unsigned i=0;i<4;i++)raw[20+i]=raw[16+i];
 for(unsigned group=0;group<18;group++){
  unsigned base=24+group*128;for(unsigned i=0;i<16;i++)raw[base+i]=uint8_t(header);
  for(unsigned i=16;i<128;i++)raw[base+i]=uint8_t(packed);
 }
 return raw;
}
int main(){try{
 ps1::XaAudio x;ck(x.decode(sector()),"decode 4-bit stereo");
 ck(x.pcm.size()==2352&&x.pcm[0][0]==4096&&x.pcm[0][1]==-4096,"stereo nibble order and 37800 to 44100 count");
 x=ps1::XaAudio{};ck(x.decode(sector(0)),"mono decode");ck(x.pcm.size()==4704&&x.pcm[0][0]==4096&&x.pcm[33][0]==-4096,"mono sound-unit order and sample count");
 for(const auto& f:x.pcm)ck(f[0]==f[1],"mono duplicates channels");
 x=ps1::XaAudio{};ck(x.decode(sector(4))&&x.pcm.size()==9408,"18900 mono sample count");
 auto eight=sector(0x11,0);for(unsigned group=0;group<18;group++)for(unsigned n=0;n<28;n++)for(unsigned ch=0;ch<4;ch++)eight[24+group*128+16+n*4+ch]=(ch&1)?0xe0:0x20;
 x=ps1::XaAudio{};ck(x.decode(eight)&&x.pcm.size()==1176&&x.pcm[0][0]==8192&&x.pcm[0][1]==-8192,"8-bit stereo byte order/sign/scale/count");
 x=ps1::XaAudio{};x.decode(sector(1,0x11,0x10));ck(x.pcm[0][0]==4096&&x.pcm[1][0]==7387,"predictor filter 1 and functional linear interpolation golden");
 x=ps1::XaAudio{};x.decode(sector(1,0x77,0x10));ck(x.previous[0]==32767,"ADPCM saturation");
 x=ps1::XaAudio{};x.decode(sector(1,0x11,15));ck(x.pcm[0][0]==8,"reserved shift maps to nine");
 x=ps1::XaAudio{};ck(!x.decode(sector(0x21))&&x.invalid==1&&x.pcm.empty(),"reserved coding tracked without fabricated PCM");
 for(unsigned i=0;i<5;i++){x.decode(sector(4));}
 ck(x.pcm.size()==22050&&x.dropped==24990,"bounded XA FIFO");
 const auto dir=std::filesystem::temp_directory_path()/"ps1_lab_xa_v21";std::filesystem::create_directories(dir);
 {std::ofstream f(dir/"xa.bin",std::ios::binary);auto write=[&](const auto& raw){f.write(reinterpret_cast<const char*>(raw.data()),raw.size());};
  auto raw=sector();raw[17]=3;raw[21]=3;write(raw);write(sector());raw=sector();raw[18]=raw[22]=8;raw[24]=0x42;write(raw);
 }
 std::ofstream(dir/"xa.cue")<<"FILE \"xa.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\n";
 ps1::Bus b;b.cd.mount(dir/"xa.cue");auto& cd=b.cd;cd.mode=0xc8;cd.filter_file=1;cd.filter_channel=2;cd.reading=true;
 cd.tick();ck(cd.lba==1&&!cd.flag&&cd.xa.pcm.empty(),"mismatched realtime XA filtered, no data IRQ");
 cd.sector_delay=0;cd.tick();ck(cd.lba==2&&!cd.flag&&cd.xa.sectors==1&&cd.data.empty()&&!cd.sector_pending,"matched XA decoded independently of CPU data FIFO");
 cd.active_volume={128,0,128,0};auto pcm=cd.audio_sample();ck(pcm[0]==4096&&pcm[1]==-4096,"XA routes through ATV matrix");
 cd.write(0,3);cd.write(3,1);pcm=cd.audio_sample();ck(!pcm[0]&&!pcm[1]&&cd.xa.pcm.size()==2350,"ADPMUTE consumes XA without audible output");cd.write(3,0);cd.write(0,0);
 cd.sector_delay=0;cd.tick();ck(cd.flag==1&&cd.sector_buffer[0]==0x42,"interleaved data sector still delivered to CPU");
 cd.reset_controller();ck(cd.xa.pcm.empty()&&cd.xa.sectors==0,"reset discards stale stream/history");
 std::filesystem::remove_all(dir);
 std::cout<<"PASS: XA 4/8-bit mono/stereo, 37800/18900 rates, predictors/sign/saturation, bounded queue, file/channel filter, interleaved data and mute\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
