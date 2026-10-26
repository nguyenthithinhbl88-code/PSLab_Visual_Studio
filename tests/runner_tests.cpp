#include "runner.hpp"
#include <chrono>
#include <stdexcept>
#include <iostream>
void ck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
 ps1::Bus b;ps1::Cpu c(b);std::mutex machine;c.reset(0);b.write32(0,0x08000000);b.write32(4,0);
 ps1::Runner runner(c,machine);runner.start(false);
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);uint64_t steps=0;
 while(std::chrono::steady_clock::now()<deadline){{std::lock_guard<std::mutex> lock(machine);steps=c.instructions;}if(steps>=2048)break;std::this_thread::yield();}
 runner.stop();ck(c.instructions>=2048&&!runner.active(),"worker progresses and joins");steps=c.instructions;
 std::this_thread::sleep_for(std::chrono::milliseconds(2));ck(c.instructions==steps,"stopped state stable");
 runner.start();std::this_thread::sleep_for(std::chrono::milliseconds(20));runner.stop();ck(c.instructions>steps,"paced restart progresses");
 c.reset(0x1e000000);runner.start(false);deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
 while(runner.active()&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
 runner.stop();ck(!runner.error().empty(),"worker reports unsupported fetch");
 c.reset(0);runner.start(false);ck(runner.error().empty(),"restart clears failure");
 for(unsigned i=0;i<10;i++){std::lock_guard<std::mutex> lock(machine);b.sio[0].buttons=uint16_t(i);b.spu.audio.clear();}
 runner.stop();std::cout<<"PASS: worker progress, join, paced restart, exception delivery and locked input/audio snapshots\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
