#pragma once
#include <array>
#include <cstdint>
namespace ps1 {
// Functional fixed-point GTE. Pipeline/busy timing is not yet cycle accurate.
struct Gte {
 std::array<uint32_t,32> data{},control{};
 uint64_t commands=0;
 uint32_t last_command=0;
 uint32_t read_data(unsigned index)const;
 void write_data(unsigned index,uint32_t value);
 uint32_t read_control(unsigned index)const;
 void write_control(unsigned index,uint32_t value);
 bool execute(uint32_t opcode);
private:
 int64_t wrap44(unsigned index,int64_t value);
 int64_t mac(unsigned index,int64_t value,unsigned shift);
 int ir(unsigned index,int64_t value,bool positive);
 int saturated(int64_t value,int low,int high,unsigned flag);
 void mac0(int64_t value);
 void finish_flags();
 std::array<int,3> vector(unsigned index)const;
 std::array<int64_t,3> multiply(unsigned matrix,const std::array<int,3>& v,unsigned translation);
 void apply(const std::array<int64_t,3>& v,unsigned shift,bool positive);
 void push_rgb();
 void depth(const std::array<int64_t,3>& base,unsigned shift,bool positive);
 void project(unsigned vector_index,unsigned shift,bool positive,bool last);
 unsigned divide(unsigned numerator,unsigned denominator);
};
}
