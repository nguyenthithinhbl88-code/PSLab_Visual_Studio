#pragma once
#include "core.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
namespace ps1 {
// Owner must stop/join before replacing Cpu/Bus or directly mutating machine state.
class Runner {
 Cpu& cpu;std::mutex& machine;
 std::thread worker;std::atomic<bool> quit{false},alive{false};
 std::mutex wait_mutex;std::condition_variable wake;
 std::string failure;
public:
 Runner(Cpu& c,std::mutex& m):cpu(c),machine(m){}
 ~Runner(){stop();}
 void start(bool paced=true);
 void stop();
 bool active()const{return alive.load();}
 std::string error(){std::lock_guard<std::mutex> lock(machine);return failure;}
};
}
