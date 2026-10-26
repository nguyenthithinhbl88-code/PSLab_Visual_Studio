#include "core.hpp"
#include "wav.hpp"
#include <iostream>
int main(int argc,char** argv){try{
 const std::string path=argc>1?argv[1]:"spu_test.wav";
 ps1::Bus b;ps1::install_spu_demo(b);ps1::WaveWriter writer(path);
 for(unsigned i=0;i<44100;i++){b.spu.render_sample();if(i%512==511)writer.append(b.spu.audio);}
 writer.append(b.spu.audio);writer.finish();std::cout<<"Wrote original 1-second stereo SPU test: "<<path<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
