#include "ps2_ee.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
int main(int argc,char** argv){
 auto ee=std::make_unique<ps2::Ee>();
 try{
  ee->demo();uint64_t steps=1000;
  for(int i=1;i<argc;i++){
   std::string arg=argv[i];
   if(arg=="--chain-demo"){ee->chain_demo();continue;}
   if(arg=="--dma-demo"){ee->dma_demo();continue;}
   if(arg=="--gpu-demo"){ee->gpu_demo();continue;}
   if(arg=="--gif-demo"){ee->gif_demo();continue;}
   if(arg=="--steps"&&i+1<argc){std::string value=argv[++i];size_t end=0;if(value.empty()||value[0]=='-')throw std::runtime_error("Invalid step budget");steps=std::stoull(value,&end);if(end!=value.size()||steps>10000000)throw std::runtime_error("Step budget must be 0..10000000");continue;}
   bool bios=arg=="--bios";
   if(bios){if(++i>=argc)throw std::runtime_error("--bios needs a file");arg=argv[i];}
   else if(arg.rfind("--",0)==0)throw std::runtime_error("Usage: pslab_ps2 [file.elf | --bios bios.bin | --gif-demo | --gpu-demo | --dma-demo | --chain-demo] [--steps N]");
   std::ifstream f(arg,std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||f.tellg()>64*1024*1024)throw std::runtime_error("Input unavailable or exceeds 64 MiB");std::vector<uint8_t> b(static_cast<size_t>(f.tellg()));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),std::streamsize(b.size())))throw std::runtime_error("Input read failed");if(bios)ee->load_bios(b);else ee->load_elf(b);
  }
  for(uint64_t n=0;n<steps;n++)ee->step();
  std::cout<<ee->diagnostic();return 0;
 }catch(const std::exception& e){std::cerr<<"Stopped: "<<e.what()<<'\n'<<ee->diagnostic();return 1;}
}
