#include "core.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
void ck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::array<int16_t,64> scales(){std::array<int16_t,64> s{};for(unsigned z=0;z<8;z++)for(unsigned x=0;x<8;x++)s[z*8+x]=int16_t(std::lround(32768.0*(z?1.0:1.0/std::sqrt(2.0))*std::cos((2*x+1)*z*3.14159265358979323846/16)));return s;}
void init(ps1::Mdec& m){m.write_control(0x80000000);m.write(0x40000001);for(unsigned i=0;i<32;i++)m.write(0x01010101);m.write(0x60000000);const auto s=scales();for(unsigned i=0;i<64;i+=2)m.write(uint16_t(s[i])|(uint32_t(uint16_t(s[i+1]))<<16));}
uint32_t block(int dc){return 0xfe000000u|(uint32_t(dc)&1023);}
std::vector<uint32_t> decode(ps1::Mdec& m,unsigned depth,bool sign,bool stp,const std::vector<int>& dc){
 m.write((1u<<29)|(depth<<27)|(sign?1u<<26:0)|(stp?1u<<25:0)|uint32_t(dc.size()));
 for(auto value:dc){m.write(block(value));}
 std::vector<uint32_t> result;for(unsigned n=0;n<10000&&!m.output.empty();n++){result.push_back(m.read());}return result;
}
int main(){try{
 ps1::Bus b;ps1::Cpu cpu(b);
 // Real screenshot: SW of reset control at 1F801824 used to hit the unmapped bus.
 cpu.reset(0x8012d268);cpu.r[2]=0x80000000;cpu.r[3]=0x1f801824;
 b.write32(cpu.pc,0xac620000);b.write32(cpu.pc+4,0x8c640000);b.write32(cpu.pc+8,0);cpu.step();cpu.step();cpu.step();
 ck(cpu.r[4]==0x80040000&&b.read32(0xbf801824)==0x80040000,"MIPS MDEC reset/control MMIO and uncached alias");
 auto& m=b.mdec;init(m);ck(m.quant_y[0]==1&&m.quant_uv[63]==1&&m.scale==scales(),"quantization and scale uploads");
 ck((m.status()&65535)==65535&&!(m.status()&(1u<<29)),"parameter count and busy after table completion");
 auto r=decode(m,1,false,false,{0});ck(r.size()==16&&std::all_of(r.begin(),r.end(),[](auto v){return v==0x80808080;}),"8-bit monochrome neutral block");
 r=decode(m,1,true,false,{0});ck(r.size()==16&&std::all_of(r.begin(),r.end(),[](auto v){return v==0;}),"signed monochrome output");
 r=decode(m,0,false,false,{0});ck(r.size()==8&&std::all_of(r.begin(),r.end(),[](auto v){return v==0x88888888;}),"4-bit monochrome packing");
 r=decode(m,1,false,false,{32});ck(r.size()==16&&std::all_of(r.begin(),r.end(),[](auto v){return v==0x88888888;}),"IDCT DC gain and unsigned bias");
 r=decode(m,3,false,false,{0,0,0,0,0,0});ck(r.size()==128&&std::all_of(r.begin(),r.end(),[](auto v){return v==0x42104210;}),"RGB555 neutral macroblock");
 r=decode(m,3,false,true,{0,0,0,0,0,0});ck(r.size()==128&&r.front()==0xc210c210&&r.back()==0xc210c210,"RGB555 bit15 control");
 r=decode(m,2,false,false,{0,0,0,0,0,0});ck(r.size()==192&&std::all_of(r.begin(),r.end(),[](auto v){return v==0x80808080;}),"RGB24 neutral macroblock");
 r=decode(m,2,false,false,{32,0,0,0,0,0});
 std::vector<uint8_t> rgb;for(auto word:r)for(unsigned lane=0;lane<4;lane++)rgb.push_back(uint8_t(word>>(lane*8)));
 for(unsigned n=0;n<rgb.size();n+=3){ck(rgb[n]==139&&rgb[n+1]==122&&rgb[n+2]==128,"Cr chroma conversion uses R/G coefficients");}
 // A custom diagonal scale matrix makes the inverse transform exactly coefficient/16.
 m.write(0x60000000);for(unsigned n=0;n<64;n+=2){const uint16_t a=n/8==n%8?0x4000:0,bv=(n+1)/8==(n+1)%8?0x4000:0;m.write(uint32_t(a)|(uint32_t(bv)<<16));}
 m.write(0x28000002);m.write(0x04402000);m.write(0xfe00fe00);unsigned index=0;
 while(!m.output.empty()){ck(m.read()==(index++==2?0x80808084:0x80808080),"AC run/quantization/zigzag/custom scale golden position");}
 ck(index==16,"custom scale output size");init(m);
 // Split a block across parameter words, including initial FE00 padding.
 m.write(0x28000002);m.write(0x0020fe00);ck(m.output.empty()&&(m.status()&(1u<<29)),"incomplete RLE waits for next input word");m.write(0xfe00fe00);
 ck(m.output.size()==16,"RLE resumes across DMA-sized words");while(!m.output.empty())ck(m.read()==0x88888888,"split RLE matches unsplit block");
 // Quantization path (qscale=1), then a run crossing the 64-coefficient end.
 m.write(0x28000001);m.write(0xfe000440);ck(m.output.size()==16,"quantized DC block complete");while(!m.output.empty())ck(m.read()==0x88888888,"quantized DC gain");
 m.write(0x28000002);m.write(0xfc000000);m.write(0xfe00fe00);ck(m.output.size()==16,"long run terminates without indexing outside the block");while(!m.output.empty())m.read();
 m.write_control(0xe0000000);ck(m.status()==0x90040000&&m.output.empty()&&m.quant_y[0]==1&&m.scale==scales(),"reset aborts FIFOs, resets status, retains tables and enables requested DMA pins");
 m.write_control(0);ck(!m.dma_in()&&!m.dma_out(),"MDEC DMA request gates");
 // DMA1 armed before compressed data: exact 16x16 row reordering through DMA0/1.
 m.write_control(0x60000000);b.dpcr=0x88;b.write32(0x1f8010f4,0x00830000);
 m.write(0x38000006);const std::vector<int> quadrants={0,0,-256,-128,128,256};for(unsigned n=0;n<6;n++)b.write32(0x2000+n*4,block(quadrants[n]));
 b.write32(0x1f801090,0x10000);b.write32(0x1f801094,0x00040020);b.write32(0x1f801098,0x01000200);
 ck(b.dma[1].control&(1u<<24),"DMA1 waits for output request");
 b.write32(0x1f801080,0x2000);b.write32(0x1f801084,6);b.write32(0x1f801088,0x11000001);b.tick_timers();
 ck(!(b.dma[0].control&(1u<<24))&&!(b.dma[1].control&(1u<<24))&&(b.dicr&0x83000000u)==0x83000000u,"DMA0/DMA1 complete and DICR edges");
 const unsigned gray[]={64,96,160,192};for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
  const unsigned v=gray[(y/8)*2+x/8]>>3;const uint16_t expected=uint16_t(v|(v<<5)|(v<<10));ck(b.read16(0x10000+(y*16+x)*2)==expected,"DMA1 scatters four 8x8 blocks into one row-major RGB555 macroblock");
 }
 // Two RGB24 macroblocks and request-mode DMA that cross FIFO backpressure.
 m.write_control(0x80000000);m.write_control(0x60000000);m.write(0x3000000c);
 for(unsigned n=0;n<12;n++)b.write32(0x3000+n*4,block(n>=8?64:0));
 b.write32(0x1f801090,0x12000);b.write32(0x1f801094,0x000c0020);b.write32(0x1f801098,0x01000200);
 b.write32(0x1f801080,0x3000);b.write32(0x1f801084,0x0001000c);b.write32(0x1f801088,0x01000201);
 for(unsigned n=0;n<10&&(b.dma[1].control&(1u<<24));n++)b.tick_timers();
 ck(!(b.dma[1].control&(1u<<24)),"two-macroblock DMA resumes after backpressure");
 for(unsigned n=0;n<768;n++){ck(b.ram[0x12000+n]==128,"RGB24 first macroblock");ck(b.ram[0x12300+n]==144,"RGB24 second macroblock");}
 // Reset while a block is unfinished must leave no stale output.
 m.write(0x28000002);m.write(0x00000000);m.write_control(0x80000000);ck(m.status()==0x80040000&&m.input.empty()&&m.output.empty(),"reset cancels partial decode");
 ck(cpu.diagnostic().find("MDEC status=")!=std::string::npos,"MDEC diagnostics are included");
 std::cout<<"PASS: MIPS reset MMIO, tables/counts/status, incremental RLE/IDCT, mono4/8 and RGB15/24, signed/STP, DMA0/1 gates/backpressure/reordering/DICR and reset cancellation\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
