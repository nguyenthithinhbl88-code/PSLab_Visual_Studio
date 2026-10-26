#include "mdec.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>
namespace ps1 {
namespace {
constexpr uint8_t order[64]={0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63};
int sx10(uint16_t v){return (v&512)?int(v&1023)-1024:int(v&1023);}
int sx9(int32_t v){const auto n=uint32_t(v)&511;return n&256?int(n)-512:int(n);}
int64_t floor_div(int64_t value,int64_t divisor){return value>=0?value/divisor:-((-value+divisor-1)/divisor);}
uint8_t pixel(int value,bool is_signed){const int clipped=std::clamp(value,-128,127);return uint8_t(clipped+(is_signed?0:128));}
}
void Mdec::reset(){
 control=command=remaining=last_read=0;idle_count=0;operation=depth=block=coefficient=qscale=table_index=0;
 signed_output=set_bit15=started=false;input.clear();output.clear();coefficients.fill(0);resets++;
 // Quantization and scale RAM are retained by a device reset.
}
void Mdec::write_control(uint32_t value){if(value&0x80000000u)reset();control=value&0x60000000u;}
bool Mdec::can_write()const{return operation?(remaining&&input.size()<=62):output.empty();}
uint32_t Mdec::status()const{
 const unsigned current=output.empty()?((depth<2)?4:(block+4)%6):output.front().block;
 uint32_t value=(operation?uint16_t(remaining-1):idle_count)|(current<<16)|(depth<<25)|(signed_output?1u<<24:0)|(set_bit15?1u<<23:0);
 if(output.empty())value|=1u<<31;
 if(!can_write())value|=1u<<30;
 if(operation)value|=1u<<29;
 if(dma_in())value|=1u<<28;
 if(dma_out())value|=1u<<27;
 return value;
}
void Mdec::write(uint32_t value){
 if(!can_write())throw std::runtime_error("MDEC input FIFO full or output pending");
 words_in++;
 if(!operation){
  command=value;commands++;idle_count=0xffff;depth=(value>>27)&3;signed_output=value&(1u<<26);set_bit15=value&(1u<<25);
  operation=value>>29;table_index=block=coefficient=qscale=0;started=false;coefficients.fill(0);
  if(operation==1)remaining=value&65535;
  else if(operation==2)remaining=(value&1)?32:16;
  else if(operation==3)remaining=32;
  else {operation=0;remaining=0;idle_count=uint16_t(value);return;}
  if(!remaining)operation=0;
  return;
 }
 remaining--;
 if(operation==2){
  for(unsigned lane=0;lane<4;lane++,table_index++){
   auto& target=table_index<64?quant_y:quant_uv;target[table_index&63]=uint8_t(value>>(lane*8));
  }
  if(!remaining)operation=0;
 }else if(operation==3){
  scale[table_index++]=int16_t(value&65535);scale[table_index++]=int16_t(value>>16);if(!remaining)operation=0;
 }else{
  input.push_back(uint16_t(value));input.push_back(uint16_t(value>>16));pump();
 }
}
void Mdec::transform(std::array<int32_t,64>& values)const{
 std::array<int32_t,64> work{};
 std::array<int32_t,64> factors{};
 for(unsigned i=0;i<64;i++)factors[i]=int32_t(floor_div(scale[i],8));
 for(unsigned pass=0;pass<2;pass++){
  for(unsigned row=0;row<8;row++){
   std::array<unsigned,8> indices{};std::array<int32_t,8> nonzero{};unsigned count=0;
   for(unsigned f=0;f<8;f++)if(values[row+f*8]){indices[count]=f*8;nonzero[count++]=values[row+f*8];}
   for(unsigned col=0;col<8;col++){
    int64_t sum=0;for(unsigned n=0;n<count;n++)sum+=int64_t(nonzero[n])*factors[col+indices[n]];
    work[col+row*8]=int32_t(floor_div(sum+4095,8192));
   }
  }
  values=work;
 }
}
void Mdec::pump(){
 if(operation!=1)return;
 while(!input.empty()&&output.empty()){
  const uint16_t code=input.front();input.pop_front();
  if(!started){
   if(code==0xfe00)continue;
   started=true;coefficient=0;qscale=code>>10;coefficients.fill(0);
  }else{
   coefficient+=(code>>10)+1;
   if(coefficient>=64){
    transform(coefficients);decoded[block]=coefficients;started=false;
    if(depth<2||++block==6){emit();macroblocks++;block=0;}
    continue;
   }
  }
  const auto& table=(depth>=2&&block<2)?quant_uv:quant_y;
  int value;
  if(!qscale)value=sx10(code)*2;
  else if(!coefficient)value=sx10(code)*table[0];
  else value=int(floor_div(int64_t(sx10(code))*table[coefficient]*qscale+4,8));
  coefficients[qscale?order[coefficient]:coefficient]=std::clamp(value,-1024,1023);
 }
 if(!remaining&&input.empty())operation=0;
}
void Mdec::emit(){
 const unsigned blocks=depth<2?1:4;
 for(unsigned tile=0;tile<blocks;tile++){
  std::vector<uint8_t> bytes;bytes.reserve(depth==2?192:128);
  for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
   const int lum=sx9(decoded[depth<2?0:tile+2][y*8+x]);
   if(depth<2){
    const uint8_t value=pixel(lum,signed_output);
    if(depth==1)bytes.push_back(value);
    else if((x&1)==0)bytes.push_back(value>>4);
    else bytes.back()|=value&0xf0;
   }else{
    const unsigned chroma=((tile&1)*8+x)/2+(((tile>>1)*8+y)/2)*8;
    const int cr=sx9(decoded[0][chroma]),cb=sx9(decoded[1][chroma]);
    const uint8_t red=pixel(lum+int(floor_div(1436ll*cr,1024)),signed_output);
    const uint8_t green=pixel(lum+int(floor_div(-352ll*cb-731ll*cr,1024)),signed_output);
    const uint8_t blue=pixel(lum+int(floor_div(1815ll*cb,1024)),signed_output);
    if(depth==2){bytes.push_back(red);bytes.push_back(green);bytes.push_back(blue);}
    else {const uint16_t rgb=uint16_t((red>>3)|((green>>3)<<5)|((blue>>3)<<10)|(set_bit15?0x8000:0));bytes.push_back(uint8_t(rgb));bytes.push_back(uint8_t(rgb>>8));}
   }
  }
  for(unsigned at=0;at<bytes.size();at+=4){
   Word word;for(unsigned lane=0;lane<4;lane++)word.value|=uint32_t(bytes[at+lane])<<(lane*8);
   word.block=depth<2?4:uint8_t(tile);word.end=tile+1==blocks&&at+4==bytes.size();
   word.macro_bytes=uint16_t(bytes.size()*blocks);
   if(depth<2)word.offset=uint16_t(at);
   else {const unsigned pixel_bytes=depth==2?3:2,row=at/(8*pixel_bytes),column=at%(8*pixel_bytes);word.offset=uint16_t((((tile>>1)*8+row)*16+(tile&1)*8)*pixel_bytes+column);}
   output.push_back(word);
  }
 }
}
Mdec::Word Mdec::read_dma(){
 if(output.empty())return {last_read};
 const Word word=output.front();output.pop_front();last_read=word.value;words_out++;
 if(output.empty()){pump();}return word;
}
uint32_t Mdec::read(){return read_dma().value;}
}
