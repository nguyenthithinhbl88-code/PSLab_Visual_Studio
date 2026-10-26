#include "ps2_ee.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
namespace ps2 {
namespace {
uint32_t phys(uint32_t a){if(a>=0x80000000&&a<0xc0000000)return a&0x1fffffff;if(a>=0x20000000&&a<0x22000000)return a-0x20000000;if(a>=0x30100000&&a<0x32000000)return a-0x30000000;return a;}
uint64_t sx32(uint32_t a){return uint64_t(int64_t(int32_t(a)));}
std::runtime_error fault(const char* s,uint32_t a){std::ostringstream o;o<<s<<" @ 0x"<<std::hex<<a;return std::runtime_error(o.str());}
void validate_memory_write(const Ee& ee,uint32_t a,unsigned n){
 for(unsigned i=0;i<n;i++){
  const uint32_t address=a+i,p=phys(address);
  if(p<ee.ram.size()||(address>=0x70000000&&address<0x70004000))continue;
  throw fault("PS2 unmapped/read-only write",address);
 }
}
}
void Ee::reset(uint32_t entry){r={};hi=lo={};cop0={};cop0[1]=47;tlb={};random_reloaded=false;cop0[12]=0x70400004;cop0[15]=0x2e20;cop0[16]=0x440;gs.reset();reset_entry=entry;mch_ricm=mch_drd=rdram_device=0;boot_f500=0;boot_f500_writes=0;reset_hardware();console.clear();first_fault.clear();trace.clear();pc=entry;next=pc+4;instructions=0;delay=false;}
uint8_t Ee::read8(uint32_t a)const{
 if(a>=0x70000000&&a<0x70004000)return scratch[a-0x70000000];
 auto p=phys(a);if(p<ram.size())return ram[p];if(p>=0x1fc00000&&uint64_t(p)<0x20000000ull)return bios[p-0x1fc00000];
 throw fault("PS2 unmapped read",a);
}
void Ee::write8(uint32_t a,uint8_t v){if(a>=0x70000000&&a<0x70004000){scratch[a-0x70000000]=v;return;}auto p=phys(a);if(p<ram.size()){ram[p]=v;return;}throw fault("PS2 unmapped/read-only write",a);}
uint64_t Ee::read(uint32_t a,unsigned n)const{if((n!=1&&n!=2&&n!=4&&n!=8)||(a&(n-1)))throw fault("PS2 invalid/unaligned read",a);auto p=phys(a);if(p>=0x12000000&&p<0x12002000)return gs.read(p,n);
 uint32_t mmio=0;if(n==4&&mmio_read(p,mmio))return mmio;
 if(n==4&&p==0x1000f500)return boot_f500;
 if(n==4&&p==0x1000f430)return 0; // Command busy bit clears immediately in this functional model.
 if(n==4&&p==0x1000f440){unsigned sa=(mch_ricm>>16)&0xfff,sop=(mch_ricm>>6)&15;if(sop)return 0;switch(sa){case 0x21:if(rdram_device<2){rdram_device++;return 0x1f;}return 0;case 0x23:return 0x0d0d;case 0x24:return 0x0090;case 0x40:return mch_ricm&31;default:return 0;}}
 uint64_t v=0;for(unsigned i=0;i<n;i++)v|=uint64_t(read8(a+i))<<(i*8);return v;}
void Ee::write(uint32_t a,uint64_t v,unsigned n){if((n!=1&&n!=2&&n!=4&&n!=8)||(a&(n-1)))throw fault("PS2 invalid/unaligned write",a);auto p=phys(a);if(p>=0x12000000&&p<0x12002000){gs.write(p,v,n);return;}
 if(p==0x1000f180&&n==1){if(console.size()==16384)console.erase(0,1);char c=char(v);console+= (c=='\n'||c=='\r'||c=='\t'||(uint8_t(c)>=32&&uint8_t(c)<127))?c:'.';return;}
 if(n==4&&mmio_write(p,uint32_t(v)))return;
 if(n==4&&p==0x1000f500){boot_f500=uint32_t(v);boot_f500_writes++;return;}
 if(n==4&&p==0x1000f430){if(((v>>16)&0xfff)==0x21&&((v>>6)&15)==1&&!(mch_drd&0x80))rdram_device=0;mch_ricm=uint32_t(v)&0x7fffffff;return;}
 if(n==4&&p==0x1000f440){mch_drd=uint32_t(v);return;}
 validate_memory_write(*this,a,n);
 for(unsigned i=0;i<n;i++){write8(a+i,uint8_t(v>>(i*8)));}
}
void Ee::write128(uint32_t address,Register value){
 auto base=address&~15u,p=phys(base);
 if(p>=0x10006000&&p<0x10007000){if((gif_ctrl&8)||(gif_mode&1))throw fault("CPU GIF write while stopped/masked: FIFO queue not implemented",p);gs.gif_push(value.low,value.high);return;}
 // Validate writability without performing reads or causing MMIO read side effects.
 validate_memory_write(*this,base,16);
 write(base,value.low,8);write(base+8,value.high,8);
}
void Ee::step(){
 if(!first_fault.empty())throw std::runtime_error("EE stopped; reset/load required: "+first_fault);
 update_interrupts();if(take_interrupt())return;
 std::ostringstream line;line<<std::hex<<pc;
 try{line<<" ins="<<read(pc,4)<<" delay="<<delay;}catch(...){line<<" fetch fault";}
 trace.push_back(line.str());if(trace.size()>64)trace.pop_front();
 try{execute();hardware_tick();}catch(const std::exception& e){first_fault=e.what();throw;}
}
void Ee::execute(){
 const auto ip=pc,ins=uint32_t(read(pc,4));const unsigned op=ins>>26,s=(ins>>21)&31,t=(ins>>16)&31,d=(ins>>11)&31,sh=(ins>>6)&31;
 const auto a=r[s].low,b=r[t].low;const auto imm=int64_t(int16_t(ins));const uint32_t address=uint32_t(a+uint64_t(imm));
 uint32_t target=next+4;bool branch=false,annul=false;
 auto put=[&](unsigned reg,uint64_t v){if(reg)r[reg].low=v;};
 auto jump=[&](uint32_t to){if(delay)throw fault("PS2 branch in delay slot unsupported",ip);branch=true;target=to;};
 auto conditional=[&](bool taken,bool likely){if(delay)throw fault("EE branch in delay slot unsupported",ip);if(likely&&!taken){annul=true;return;}jump(taken?ip+4+(uint32_t(imm)<<2):next+4);};
 switch(op){
 case 0:switch(ins&63){
  case 0:put(d,sx32(uint32_t(b)<<sh));break;case 2:put(d,sx32(uint32_t(b)>>sh));break;
  case 3:put(d,sx32(uint32_t(int32_t(b)>>sh)));break;
  case 4:put(d,sx32(uint32_t(b)<<(a&31)));break;case 6:put(d,sx32(uint32_t(b)>>(a&31)));break;case 7:put(d,sx32(uint32_t(int32_t(b)>>(a&31))));break;
  case 0x0a:if(!b)put(d,a);break;case 0x0b:if(b)put(d,a);break;
  case 0x0f:
   // SYNC.L / SYNC.P. All stores, GIF consumption and software rendering currently
   // finish synchronously before the next EE instruction; there is no pending queue.
   if(s||t||d||(sh!=0&&sh!=16))throw fault("EE SYNC encoding unsupported",ins);
   break;
  case 0x10:put(d,hi.low);break;case 0x11:hi.low=a;break;case 0x12:put(d,lo.low);break;case 0x13:lo.low=a;break;
  case 0x18:case 0x19:{const uint64_t product=(ins&63)==0x18?uint64_t(int64_t(int32_t(a))*int64_t(int32_t(b))):uint64_t(uint32_t(a))*uint32_t(b);lo.low=sx32(uint32_t(product));hi.low=sx32(uint32_t(product>>32));put(d,lo.low);break;}
  case 0x1a:{int32_t x=int32_t(a),y=int32_t(b);if(!y){lo.low=sx32(x<0?1:0xffffffffu);hi.low=sx32(uint32_t(x));}else if(x==INT32_MIN&&y==-1){lo.low=sx32(0x80000000);hi.low=0;}else{lo.low=sx32(uint32_t(x/y));hi.low=sx32(uint32_t(x%y));}break;}
  case 0x1b:{uint32_t x=uint32_t(a),y=uint32_t(b);lo.low=sx32(y?x/y:0xffffffffu);hi.low=sx32(y?x%y:x);break;}
  case 0x14:put(d,b<<(a&63));break;case 0x16:put(d,b>>(a&63));break;case 0x17:put(d,uint64_t(int64_t(b)>>(a&63)));break;
  case 0x2a:put(d,int64_t(a)<int64_t(b));break;
  case 0x38:put(d,b<<sh);break;case 0x3a:put(d,b>>sh);break;case 0x3b:put(d,uint64_t(int64_t(b)>>sh));break;
  case 0x3c:put(d,b<<(sh+32));break;case 0x3e:put(d,b>>(sh+32));break;case 0x3f:put(d,uint64_t(int64_t(b)>>(sh+32)));break;
  case 8:jump(uint32_t(a));break;case 9:jump(uint32_t(a));put(d,sx32(ip+8));break;
  case 0x21:put(d,sx32(uint32_t(a+b)));break;case 0x23:put(d,sx32(uint32_t(a-b)));break;
  case 0x24:put(d,a&b);break;case 0x25:put(d,a|b);break;case 0x26:put(d,a^b);break;case 0x27:put(d,~(a|b));break;
  case 0x2b:put(d,a<b);break;case 0x2d:put(d,a+b);break;case 0x2f:put(d,a-b);break;
  default:throw fault("PS2 EE SPECIAL not implemented",ins);
 }break;
 case 1:{if(t!=0&&t!=1&&t!=2&&t!=3&&t!=16&&t!=17&&t!=18&&t!=19)throw fault("EE REGIMM unsupported",ins);bool taken=(t&1)?int64_t(a)>=0:int64_t(a)<0;conditional(taken,t&2);if(t&16)put(31,sx32(ip+8));break;}
 case 0x14:case 0x15:conditional(op==0x14?a==b:a!=b,true);break;
 case 0x16:case 0x17:conditional(op==0x16?int64_t(a)<=0:int64_t(a)>0,true);break;
 case 2:case 3:jump(((ip+4)&0xf0000000)|((ins&0x3ffffff)<<2));if(op==3)put(31,sx32(ip+8));break;
 case 4:case 5:jump((op==4?a==b:a!=b)?ip+4+(uint32_t(imm)<<2):next+4);break;
 case 6:case 7:jump((op==6?int64_t(a)<=0:int64_t(a)>0)?ip+4+(uint32_t(imm)<<2):next+4);break;
 case 0x10:{
  if(ins==0x42000018){if(delay)throw fault("ERET in branch delay slot unsupported",ip);pc=cop0[12]&4?cop0[30]:cop0[14];cop0[12]&=~(cop0[12]&4?4u:2u);next=pc+4;delay=false;r[0]={};instructions++;cop0[9]++;return;}
  if(tlb_operation(ins))break;
  const bool supported=cop0_supported(d);
  if((ins&0x7ff)||!supported)throw fault("EE COP0 register/encoding unsupported",ins);
  if(s==0)put(t,sx32(cop0[d]));else if(s==4)cop0_write(d,uint32_t(b));else throw fault("EE COP0 operation unsupported",ins);
  break;
 }
 case 9:put(t,sx32(uint32_t(a+uint64_t(imm))));break;
 case 0x19:put(t,a+uint64_t(imm));break;
 case 0x0a:put(t,int64_t(a)<imm);break;
 case 0x0b:put(t,a<uint64_t(imm));break;
 case 0x0c:put(t,a&(ins&65535));break;case 0x0d:put(t,a|(ins&65535));break;case 0x0e:put(t,a^(ins&65535));break;
 case 0x0f:put(t,sx32((ins&65535)<<16));break;
 case 0x20:put(t,uint64_t(int64_t(int8_t(read(address,1)))));break;
 case 0x21:put(t,uint64_t(int64_t(int16_t(read(address,2)))));break;
 case 0x23:put(t,sx32(uint32_t(read(address,4))));break;
 case 0x27:put(t,uint32_t(read(address,4)));break;
 case 0x24:put(t,read(address,1));break;case 0x25:put(t,read(address,2));break;
 case 0x28:write(address,b,1);break;case 0x29:write(address,b,2);break;case 0x2b:write(address,b,4);break;
 case 0x37:put(t,read(address,8));break;case 0x3f:write(address,b,8);break;
 case 0x1e:{const auto base=address&~15u;auto lo=read(base,8),hi=read(base+8,8);if(t)r[t]={lo,hi};break;}
 case 0x1f:{write128(address,r[t]);break;}
 default:throw fault("PS2 EE opcode not implemented",ins);
 }
 if(annul){pc=ip+8;next=ip+12;delay=false;}else{pc=next;next=target;delay=branch;}r[0]={};instructions++;cop0[9]++;
}
void Ee::demo(){std::fill(ram.begin(),ram.end(),0);reset();const uint32_t code[]={0x24080007,0x24090005,0x01091021,0xac020200,0x08040004,0};for(unsigned i=0;i<6;i++)write(pc+i*4,code[i],4);}
void Ee::register_demo(){demo();const uint32_t code[]={0x3c081200,0x3409bb55,0xfd0900e0,0x400a7800,0x240b0007,0x240c0005,0x016c0018,0x00001012,0xac020200,0x08040009,0};for(unsigned n=0;n<11;n++)write(0x100000+n*4,code[n],4);}
void Ee::gif_demo(){
 demo();const uint32_t code[]={0x3c081000,0x35086000,0x24090100,0x792a0000,0x7d0a0000,0x25290010,0x792a0000,0x7d0a0000,0x08040008,0};
 for(unsigned i=0;i<10;i++)write(0x100000+i*4,code[i],4);
 write128(0x100,{1ull|(1ull<<15)|(1ull<<60),14});
 write128(0x110,{0x00010001,0x4c}); // FRAME_1 via PACKED A+D.
}
void Ee::gpu_demo(){
 demo();
 // Display setup belongs to this authored test harness. Drawing travels through CPU SQ/GIF.
 write(0x12000000,1,8);write(0x12000070,10ull<<9,8);
 write(0x12000080,(639ull<<32)|(447ull<<44),8);
 auto xy=[](unsigned x,unsigned y){return uint64_t(x*16)|(uint64_t(y*16)<<16);};
 const std::pair<unsigned,uint64_t> commands[]={
  {0x1a,1},{0x4c,10ull<<16},{0x4e,1ull<<32},{0x47,0},{0x40,(639ull<<16)|(447ull<<48)},
  {0,6},{1,0x801c120c},{5,xy(0,0)},{5,xy(640,448)},
  {0,6},{1,0x80d9a833},{5,xy(50,60)},{5,xy(290,180)},
  {0,6},{1,0x8061c949},{5,xy(350,250)},{5,xy(590,390)},
  {0,3},{1,0x804075ec},{5,xy(350,70)},{5,xy(580,180)},{5,xy(320,210)},
  {0,1},{1,0x80ffffff},{5,xy(50,260)},{5,xy(290,390)},
  {0,0},{1,0x80ffffff},{5,xy(320,224)}
 };
 constexpr unsigned count=unsigned(sizeof(commands)/sizeof(commands[0]))+1;
 const uint32_t code[]={0x3c081000,0x35086000,0x24090100,0x240b0000|count,
  0x792a0000,0x7d0a0000,0x25290010,0x256bffff,0x1560fffb,0,0x0804000a,0};
 for(unsigned i=0;i<12;i++)write(0x100000+i*4,code[i],4);
 write128(0x100,{uint64_t(count-1)|(1ull<<15)|(1ull<<60),14});
 for(unsigned i=0;i<count-1;i++)write128(0x110+i*16,{commands[i].second,commands[i].first});
}
void Ee::load_bios(const std::vector<uint8_t>& bytes){if(bytes.size()!=bios.size())throw std::runtime_error("PS2 BIOS must be 4 MiB; boot not supported yet");bios=bytes;reset(0xbfc00000);}
void Ee::load_elf(const std::vector<uint8_t>& bytes){
 auto get=[&](size_t at,unsigned n){if(at>bytes.size()||n>bytes.size()-at)throw std::runtime_error("Truncated ELF");uint32_t v=0;for(unsigned i=0;i<n;i++)v|=uint32_t(bytes[at+i])<<(8*i);return v;};
 if(bytes.size()<52||get(0,4)!=0x464c457f||get(4,1)!=1||get(5,1)!=1||get(6,1)!=1||get(16,2)!=2||get(18,2)!=8||get(20,4)!=1)throw std::runtime_error("Need ELF32 little-endian MIPS executable");
 const auto entry=get(24,4),offset=get(28,4),stride=get(42,2),count=get(44,2);
 if((entry&3)||phys(entry)>=ram.size()||stride<32||count==0||count>256||uint64_t(offset)+uint64_t(stride)*count>bytes.size())throw std::runtime_error("Invalid ELF entry/program headers");
 struct Segment{uint32_t offset,address,size,memory;};std::vector<Segment> segments;bool entry_loaded=false;
 for(unsigned i=0;i<count;i++){size_t h=offset+size_t(i)*stride;if(get(h,4)!=1)continue;Segment s{get(h+4,4),phys(get(h+8,4)),get(h+16,4),get(h+20,4)};
  if(s.size>s.memory||uint64_t(s.offset)+s.size>bytes.size()||uint64_t(s.address)+s.memory>ram.size())throw std::runtime_error("ELF segment outside RAM/file");
  for(auto old:segments)if(s.memory&&old.memory&&uint64_t(s.address)<uint64_t(old.address)+old.memory&&uint64_t(old.address)<uint64_t(s.address)+s.memory)throw std::runtime_error("Overlapping ELF segments unsupported");
  if(phys(entry)>=s.address&&uint64_t(phys(entry))+4<=uint64_t(s.address)+s.size)entry_loaded=true;
  segments.push_back(s);
 }
 if(!entry_loaded)throw std::runtime_error("ELF entry must point to loaded data");
 std::vector<uint8_t> fresh(ram.size());for(auto s:segments)std::copy_n(bytes.begin()+s.offset,s.size,fresh.begin()+s.address);
 ram.swap(fresh);scratch.fill(0);reset(entry);
}
std::string Ee::diagnostic()const{std::ostringstream o;o<<"PSLab v30.1 PS2 / EE HARDWARE + TLB REGISTER MODEL\r\nBasic untextured GS; no VU/IOP/SPU2/CDVD, no games or complete BIOS boot.\r\nInstructions: "<<instructions<<"\r\nPC="<<std::hex<<pc<<" Next="<<next<<"\r\nRAM[200]="<<read(0x200,4)<<"\r\n";o<<"HI="<<std::hex<<hi.low<<" LO="<<lo.low<<"\r\n";for(unsigned n:{0u,1u,2u,3u,4u,5u,6u,8u,9u,10u,11u,12u,13u,14u,15u,16u,23u,28u,29u,30u})o<<"COP0["<<std::dec<<n<<"]="<<std::hex<<cop0[n]<<"\r\n";o<<"MCH_RICM="<<std::hex<<mch_ricm<<" MCH_DRD="<<mch_drd<<" RDRAM devices enumerated="<<rdram_device<<"\r\n";
 o<<"BOOT_F500="<<std::hex<<boot_f500<<" writes="<<std::dec<<boot_f500_writes<<" (experimental latch; hardware side effects unmodeled)\r\n";
 o<<"First stop: "<<first_fault<<"\r\nKPUTCHAR (last 16 KiB):\r\n"<<console<<"\r\nLast 64 attempted instructions:\r\n";for(const auto& t:trace)o<<t<<"\r\n";
 o<<tlb_diagnostic();o<<hardware_diagnostic();o<<gs.diagnostic();for(unsigned n=0;n<32;n++)o<<"r"<<std::dec<<n<<"="<<std::hex<<std::setw(16)<<std::setfill('0')<<r[n].high<<":"<<std::setw(16)<<r[n].low<<"\r\n";return o.str();}
}
