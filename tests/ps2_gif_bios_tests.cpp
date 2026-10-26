#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F> void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,"expected explicit fault");}
uint64_t tag(unsigned loops,unsigned nreg,unsigned format=0){return loops|(uint64_t(format)<<58)|(uint64_t(nreg&15)<<60);}
void ad(ps2::Gs& g,unsigned id,uint64_t v){g.gif_push(tag(1,1),14);g.gif_push(v,id);}
}
int main(){try{
 auto ee=std::make_unique<ps2::Ee>();ee->gif_demo();for(int i=0;i<1000;i++)ee->step();check(ee->gs.drawing[0x4c]==0x10001&&ee->gs.gif_words==2&&ee->gs.gif_packets==1,"CPU LQ/SQ -> GIF -> FRAME_1");
 ps2::Gs g;unsigned states=0;
 for(unsigned i=0;i<128;i++)if(ps2::Gs::drawing_name(i)&&i!=0x54&&i!=0x0f&&i!=0x7f&&i!=0x60&&i!=0x61&&i!=0x62){ad(g,i,0x123400000000ull+i);check(g.drawing[i]==0x123400000000ull+i,"A+D state mapping");states++;}
 check(states==50,"50 ordinary drawing/transfer state latches");
 g.reset();g.write(0x12001010,0,8);ad(g,0x60,0xff00000012ull);check(g.read(0x12001080,8)==0x12&&g.irq(),"SIGNAL masked ID and IRQ");
 ad(g,0x62,0xff00000034ull);check(g.read(0x12001080,8)==0x3400000012ull,"LABEL high ID");ad(g,0x61,0);check((g.read(0x12001000,8)&3)==3,"FINISH event");g.write(0x12001000,3,8);check(!g.irq(),"CSR acknowledge");
 ad(g,0x60,0x0f00000005ull);check(uint32_t(g.read(0x12001080,8))==0x15,"SIGNAL preserves bits outside mask");fails([&]{ad(g,0x60,0);});check(g.gif_faulted,"pending signal reports unsupported stall");
 g.reset();ad(g,0x0f,42);ad(g,0x7f,43);check(g.drawing[0x0f]==0&&g.drawing[0x7f]==0,"A+D NOP aliases");g.gif_push(tag(1,3,1),0x531);g.gif_push(0x12345678,0x9999);g.gif_push(0xabcdef,0xdeadbeef);check(g.drawing[1]==0x12345678&&g.drawing[3]==0x9999&&g.drawing[5]==0xabcdef&&!g.gif_remaining,"odd REGLIST padding not consumed as tag");
 g.gif_push(tag(1,1),14);g.gif_push(0xaa,0x4d);check(g.drawing[0x4d]==0xaa,"next packet boundary");
 g.reset();g.gif_push(tag(1,0),0xffffffffffffffffull);for(int i=0;i<16;i++)g.gif_push(0,0);check(!g.gif_remaining&&g.gif_words==17,"NREG zero means sixteen");
 g.reset();g.gif_push(tag(0,1)|(1ull<<46)|(6ull<<47),14);check(g.drawing[0]==6&&!g.gif_remaining,"PRE PRIM with zero NLOOP");
 fails([&]{g.gif_push(tag(1,1,2),0);});fails([&]{g.gif_push(tag(1,1),14);});g.reset();fails([&]{g.gif_push(tag(1,1),11);});g.reset();fails([&]{ad(g,0x54,0);});
 ee->reset();ee->write128(0xb0006010,{tag(1,1),14});ee->write128(0x10006ff0,{77,0x40});check(ee->gs.drawing[0x40]==77,"FIFO alias page and KSEG1");fails([&]{ee->write(0x10006000,0,8);});
 ee->write(0x1000f430,0x00210040,4);ee->write(0x1000f430,0x00210000,4);check(ee->read(0x1000f430,4)==0,"RDRAM command busy completion");check(ee->read(0x1000f440,4)==31&&ee->read(0x1000f440,4)==31&&ee->read(0x1000f440,4)==0,"two RDRAM devices");
 ee->write(0x1000f430,0x00230000,4);check(ee->read(0xb000f440,4)==0x0d0d,"RDRAM config alias");ee->write(0x1000f430,0x00240000,4);check(ee->read(0x1000f440,4)==0x90,"RDRAM timing response");fails([&]{ee->read(0x1000f440,2);});
 std::vector<uint8_t> original_bios(4*1024*1024); // Authored test ROM, no Sony code.
 const uint32_t program[]={0x3c081000,0x3508f180,0x24090042,0xa1090000,0x3c0a1000,0x354af430,0x3c0b0024,0xad4b0000,0x8d4c0010,0xffffffff};
 for(unsigned i=0;i<10;i++){uint32_t word=program[i];for(unsigned b=0;b<4;b++)original_bios[i*4+b]=uint8_t(word>>(b*8));}
 ee->load_bios(original_bios);for(int i=0;i<9;i++)ee->step();check(ee->console=="B"&&ee->r[12].low==0x90&&ee->pc==0xbfc00024,"BIOS reset fetch -> MMIO -> console and RDRAM");fails([&]{ee->step();});check(!ee->first_fault.empty()&&ee->trace.size()==10&&ee->instructions==9,"retained BIOS stop/trace");auto stop=ee->first_fault;fails([&]{ee->step();});check(ee->first_fault==stop&&ee->trace.size()==10,"first fault remains stable");
 ee->reset(ee->reset_entry);check(ee->pc==0xbfc00000&&ee->first_fault.empty()&&ee->console.empty(),"reset retains BIOS entry");
 std::cout<<"PASS: GIF CPU/FIFO/A+D/REGLIST, 50 states, signal/label/finish, strict faults, synthetic BIOS/RDRAM/KPUTCHAR/trace\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
