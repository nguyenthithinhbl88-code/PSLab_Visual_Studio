#include "runner.hpp"
#include <chrono>
#include <stdexcept>
namespace ps1 {
void Runner::stop(){quit=true;wake.notify_all();if(worker.joinable())worker.join();alive=false;}
void Runner::start(bool paced){
 stop();{std::lock_guard<std::mutex> lock(machine);failure.clear();}
 quit=false;alive=true;
 try{worker=std::thread([this,paced]{
  using Clock=std::chrono::steady_clock;const auto start=Clock::now();uint64_t clocks=0;
  try{
   while(!quit){
    {std::lock_guard<std::mutex> lock(machine);for(unsigned i=0;i<2048&&!quit;i++){cpu.step();clocks++;if(cpu.bios_system_error)throw std::runtime_error(cpu.failure_summary());}}
    std::this_thread::yield(); // allow UI/audio consumers to acquire the shared machine mutex
    if(paced){const auto target=start+std::chrono::nanoseconds(int64_t(double(clocks)*1e9/33868800.0));
     if(target-Clock::now()>std::chrono::milliseconds(2)){std::unique_lock<std::mutex> lock(wait_mutex);wake.wait_until(lock,target,[this]{return quit.load();});}
    }
   }
  }catch(const std::exception& e){std::lock_guard<std::mutex> lock(machine);failure=e.what();}
  alive=false;
 });}catch(...){alive=false;throw;}
}
}
