#include "ps2_ee.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v,const char* s){if(!v)throw std::runtime_error(s);}
int main(){try{
 ps2::Ee e;e.demo();for(unsigned n=0;n<1000;n++)e.step();ck(e.read(0x200,4)==12,"EE demo computes and stores 12");
 e.reset();e.r[2]={0x1122334455667788ull,0x99aabbccddeeff00ull};e.r[1].low=0x2003;e.write(e.pc,0x7c220000,4);e.write(e.pc+4,0x78230000,4);e.step();e.step();ck(e.r[3].low==e.r[2].low&&e.r[3].high==e.r[2].high,"SQ/LQ silently align and preserve 128 bits");
 e.reset();e.r[1].low=0xfffffff0;e.r[2].high=99;e.write(e.pc,0x24220020,4);e.step();ck(e.r[2].low==16&&e.r[2].high==99,"32-bit arithmetic wraps and preserves upper 64");
 e.reset();e.write(e.pc,0x10000002,4);e.write(e.pc+4,0x24020007,4);e.write(e.pc+8,0x24020009,4);e.write(e.pc+12,0x24430005,4);for(unsigned n=0;n<3;n++)e.step();ck(e.r[2].low==7&&e.r[3].low==12,"branch executes delay slot, skips fallthrough");
 e.write(0x80000200,0xabcdef01,4);ck(e.read(0xa0000200,4)==0xabcdef01,"RAM KSEG aliases");e.write(0x70003ff8,0x1122334455667788ull,8);ck(e.read(0x70003ff8,8)==0x1122334455667788ull,"scratchpad last doubleword");
 bool rejected=false;try{e.read(0x10002000,4);}catch(const std::runtime_error&){rejected=true;}ck(rejected,"missing MMIO is not fabricated");
 e.reset();e.write(e.pc,0x44000000,4);rejected=false;try{e.step();}catch(const std::runtime_error&){rejected=true;}ck(rejected&&e.pc==0x100000&&e.instructions==0,"unsupported opcode retains failing PC");
 std::vector<uint8_t> elf(0x110);auto put=[&](size_t at,uint32_t v,unsigned n=4){for(unsigned i=0;i<n;i++)elf[at+i]=uint8_t(v>>(8*i));};
 put(0,0x464c457f);put(4,1,1);put(5,1,1);put(6,1,1);put(16,2,2);put(18,8,2);put(20,1);put(24,0x100000);put(28,52);put(40,52,2);put(42,32,2);put(44,1,2);put(52,1);put(56,0x100);put(60,0x100000);put(68,8);put(72,16);put(0x100,0x2402002a);put(0x104,0xac020200);
 e.ram[0x100008]=255;e.load_elf(elf);e.step();e.step();ck(e.read(0x200,4)==42&&e.ram[0x100008]==0,"ELF load entry execution and BSS zero");
 const auto old=e.pc;auto bad=elf;bad[5]=2;rejected=false;try{e.load_elf(bad);}catch(const std::runtime_error&){rejected=true;}ck(rejected&&e.pc==old&&e.read(0x200,4)==42,"invalid ELF load is atomic");
 put(72,0xffffffff);rejected=false;try{e.load_elf(elf);}catch(const std::runtime_error&){rejected=true;}ck(rejected,"out-of-bounds segment rejected");
 std::vector<uint8_t> bios(4*1024*1024);bios[0]=0x2a;bios[1]=0;bios[2]=2;bios[3]=0x24;e.load_bios(bios);e.step();ck(e.r[2].low==42,"BIOS reset fetch mapping (not BIOS boot)");
 std::cout<<"PASS: original EE demo, 128-bit registers/LQ/SQ, arithmetic, delay slots, memory aliases, ELF/BSS/atomic validation and BIOS mapping\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
