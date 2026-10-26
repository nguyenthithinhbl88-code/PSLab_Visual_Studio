#pragma once
#include <array>
#include <cstdint>
#include <deque>
namespace ps1 {
struct Mdec {
 struct Word {uint32_t value=0;uint16_t offset=0;uint16_t macro_bytes=0;uint8_t block=4;bool end=false;};
 uint32_t control=0,command=0,remaining=0,last_read=0;
 uint16_t idle_count=0;
 unsigned operation=0,depth=0,block=0,coefficient=0,qscale=0,table_index=0;
 bool signed_output=false,set_bit15=false,started=false;
 std::array<uint8_t,64> quant_y{},quant_uv{};
 std::array<int16_t,64> scale{};
 std::array<int32_t,64> coefficients{};
 std::array<std::array<int32_t,64>,6> decoded{};
 std::deque<uint16_t> input;
 std::deque<Word> output;
 uint64_t commands=0,macroblocks=0,words_in=0,words_out=0,resets=0;
 void reset();
 void write_control(uint32_t value);
 uint32_t status()const;
 bool can_write()const;
 bool dma_in()const{return (control&(1u<<30))&&can_write();}
 bool dma_out()const{return (control&(1u<<29))&&!output.empty();}
 void write(uint32_t value);
 uint32_t read();
 Word read_dma();
 void pump();
 void transform(std::array<int32_t,64>& values)const;
 void emit();
};
}
