#include "gte.hpp"
#include <algorithm>
namespace ps1 {
namespace {
int sx16(uint32_t v){return int16_t(v);}
int64_t sx32(uint32_t v){return v<0x80000000u?int64_t(v):int64_t(v)-0x100000000ll;}
}
uint32_t Gte::read_data(unsigned n)const{
 n&=31;
 if(n==15)return data[14];
 if(n==28||n==29){uint32_t rgb=0;for(unsigned i=0;i<3;i++)rgb|=uint32_t(std::clamp(sx16(data[9+i])>>7,0,31))<<(i*5);return rgb;}
 if(n==1||n==3||n==5||(n>=8&&n<=11))return uint32_t(sx16(data[n]));
 if(n==7||(n>=16&&n<=19))return data[n]&0xffff;
 return data[n];
}
void Gte::write_data(unsigned n,uint32_t value){
 n&=31;
 if(n==31||n==29)return;
 if(n==15){data[12]=data[13];data[13]=data[14];data[14]=value;return;}
 if(n==28){for(unsigned i=0;i<3;i++)data[9+i]=((value>>(i*5))&31)*128;return;}
 if(n==30){data[30]=value;uint32_t bits=(value&0x80000000)?~value:value;unsigned count=0;for(unsigned i=0;i<32&&!(bits&0x80000000);i++){count++;bits<<=1;}data[31]=count;return;}
 if(n==1||n==3||n==5||(n>=8&&n<=11))value=uint32_t(sx16(value));
 if(n==7||(n>=16&&n<=19))value&=0xffff;
 data[n]=value;
}
uint32_t Gte::read_control(unsigned n)const{return control[n&31];}
void Gte::write_control(unsigned n,uint32_t value){
 n&=31;
 if(n==4||n==12||n==20||n==26||n==27||n==29||n==30)value=uint32_t(sx16(value));
 if(n==31){control[31]=value&0x7ffff000;finish_flags();return;}
 control[n]=value;
}
void Gte::finish_flags(){control[31]=(control[31]&0x7fffffff)|((control[31]&0x7f87e000)?0x80000000u:0);}
int Gte::saturated(int64_t v,int low,int high,unsigned flag){if(v<low||v>high)control[31]|=1u<<flag;return int(std::clamp(v,int64_t(low),int64_t(high)));}
int64_t Gte::wrap44(unsigned n,int64_t v){
 if(v>0x7ffffffffffll)control[31]|=1u<<(31-n);
 if(v< -0x80000000000ll)control[31]|=1u<<(28-n);
 const auto bits=uint64_t(v)&0xfffffffffffULL;
 return (bits&0x80000000000ULL)?int64_t(bits)-0x100000000000ll:int64_t(bits);
}
int64_t Gte::mac(unsigned n,int64_t v,unsigned shift){v=wrap44(n,v)>>shift;data[24+n]=uint32_t(v);return v;}
int Gte::ir(unsigned n,int64_t v,bool positive){int value=saturated(v,positive?0:-32768,32767,25-n);data[8+n]=uint32_t(value);return value;}
void Gte::mac0(int64_t v){if(v>0x7fffffff)control[31]|=1u<<16;if(v< -0x80000000ll)control[31]|=1u<<15;data[24]=uint32_t(v);}
std::array<int,3> Gte::vector(unsigned n)const{
 if(n==3)return {sx16(data[9]),sx16(data[10]),sx16(data[11])};
 return {sx16(data[n*2]),sx16(data[n*2]>>16),sx16(data[n*2+1])};
}
std::array<int64_t,3> Gte::multiply(unsigned matrix,const std::array<int,3>& v,unsigned translation){
 std::array<int,9> m{};
 if(matrix<3){for(unsigned i=0;i<9;i++)m[i]=sx16(control[matrix*8+i/2]>>((i&1)*16));}
 else m={-int(data[6]&255)*16,int(data[6]&255)*16,sx16(data[8]),sx16(control[1]),sx16(control[1]),sx16(control[1]),sx16(control[2]),sx16(control[2]),sx16(control[2])};
 std::array<int64_t,3> result{};
 for(unsigned row=0;row<3;row++){
  int64_t sum=translation<3?sx32(control[(translation==0?5:translation==1?13:21)+row])*4096:0;
  for(unsigned col=0;col<3;col++)sum=wrap44(row+1,sum+int64_t(m[row*3+col])*v[col]);
  result[row]=sum;
 }
 return result;
}
void Gte::apply(const std::array<int64_t,3>& values,unsigned shift,bool positive){for(unsigned i=0;i<3;i++){mac(i+1,values[i],shift);ir(i+1,sx32(data[25+i]),positive);}}
void Gte::push_rgb(){
 uint32_t rgb=data[6]&0xff000000;
 for(unsigned i=0;i<3;i++)rgb|=uint32_t(saturated(sx32(data[25+i])>>4,0,255,21-i))<<(i*8);
 data[20]=data[21];data[21]=data[22];data[22]=rgb;
}
void Gte::depth(const std::array<int64_t,3>& base,unsigned shift,bool positive){
 std::array<int64_t,3> values{};
 const int factor=sx16(data[8]);
 for(unsigned i=0;i<3;i++){
  const auto difference=mac(i+1,sx32(control[21+i])*4096-base[i],shift);
  // The intermediate FC difference always uses signed IR saturation, regardless of lm.
  const int delta=ir(i+1,sx32(uint32_t(difference)),false);
  values[i]=base[i]+int64_t(delta)*factor;
 }
 apply(values,shift,positive);push_rgb();
}
unsigned Gte::divide(unsigned h,unsigned z){
 if(h>=z*2){control[31]|=1u<<17;return 0x1ffff;}
 unsigned shift=0;for(unsigned b=z;b<0x8000;b<<=1)shift++;
 const uint32_t n=h<<shift,d=z<<shift,index=(d-0x7fc0)>>7;
 const unsigned table=unsigned(std::max(0,int((0x40000/(index+0x100)+1)/2)-0x101));
 const uint32_t u=table+0x101,d1=(0x2000080-d*u)>>8,d2=(0x80+d1*u)>>8;
 return unsigned(std::min(uint64_t(0x1ffff),(uint64_t(n)*d2+0x8000)>>16));
}
void Gte::project(unsigned n,unsigned shift,bool positive,bool last){
 const auto values=multiply(0,vector(n),0);
 for(unsigned i=0;i<3;i++)mac(i+1,values[i],shift);
 ir(1,sx32(data[25]),positive);ir(2,sx32(data[26]),positive);
 data[11]=uint32_t(std::clamp(sx32(data[27]),int64_t(positive?0:-32768),int64_t(32767)));
 // RTPS IR3 flag uses raw Z>>12 and signed range even when sf=0/lm=1.
 saturated(values[2]>>12,-32768,32767,22);
 data[16]=data[17];data[17]=data[18];data[18]=data[19];
 data[19]=uint32_t(saturated(values[2]>>12,0,65535,18));
 const unsigned quotient=divide(control[26]&0xffff,data[19]);
 const int64_t x=int64_t(quotient)*sx16(data[9])+sx32(control[24]);
 const int64_t y=int64_t(quotient)*sx16(data[10])+sx32(control[25]);
 mac0(x);mac0(y);
 const int screen_x=saturated(x>>16,-1024,1023,14),screen_y=saturated(y>>16,-1024,1023,13);
 data[12]=data[13];data[13]=data[14];data[14]=uint16_t(screen_x)|(uint32_t(uint16_t(screen_y))<<16);
 if(last){const int64_t cue=int64_t(quotient)*sx16(control[27])+sx32(control[28]);mac0(cue);data[8]=uint32_t(saturated(cue>>12,0,4096,12));}
}
bool Gte::execute(uint32_t opcode){
 const unsigned cmd=opcode&63,shift=(opcode&(1u<<19))?12:0;const bool positive=opcode&1024;
 switch(cmd){case 1:case 6:case 12:case 16:case 17:case 18:case 19:case 20:case 22:case 27:case 28:case 30:case 32:case 40:case 41:case 42:case 45:case 46:case 48:case 61:case 62:case 63:break;default:last_command=opcode;return false;}
 commands++;last_command=opcode;control[31]=0;
 if(cmd==1||cmd==48){const unsigned count=cmd==1?1:3;for(unsigned n=0;n<count;n++)project(n,shift,positive,n+1==count);}
 else if(cmd==6){
  const int64_t x0=sx16(data[12]),y0=sx16(data[12]>>16),x1=sx16(data[13]),y1=sx16(data[13]>>16),x2=sx16(data[14]),y2=sx16(data[14]>>16);
  mac0(x0*y1+x1*y2+x2*y0-x0*y2-x1*y0-x2*y1);
 }else if(cmd==45||cmd==46){
  int64_t sum=0;for(unsigned i=cmd==45?17:16;i<20;i++)sum+=data[i]&0xffff;
  const auto value=sum*sx16(control[cmd==45?29:30]);mac0(value);data[7]=uint32_t(saturated(value>>12,0,65535,18));
 }else if(cmd==18){
  const unsigned mx=(opcode>>17)&3,cv=(opcode>>13)&3;const auto v=vector((opcode>>15)&3);
  auto values=multiply(mx,v,cv);
  if(cv==2){apply(values,shift,false);auto tail=v;tail[0]=0;values=multiply(mx,tail,3);} // FC hardware quirk; discarded-stage flags are approximate.
  apply(values,shift,positive);
 }else if(cmd==40){auto v=vector(3);apply({int64_t(v[0])*v[0],int64_t(v[1])*v[1],int64_t(v[2])*v[2]},shift,positive);}
 else if(cmd==12){auto v=vector(3);const int x=sx16(control[0]),y=sx16(control[2]),z=sx16(control[4]);apply({int64_t(v[2])*y-int64_t(v[1])*z,int64_t(v[0])*z-int64_t(v[2])*x,int64_t(v[1])*x-int64_t(v[0])*y},shift,positive);}
 else if(cmd==61||cmd==62){auto v=vector(3);std::array<int64_t,3> values{};for(unsigned i=0;i<3;i++)values[i]=int64_t(v[i])*sx16(data[8])+(cmd==62?sx32(data[25+i])*(int64_t(1)<<shift):0);apply(values,shift,positive);push_rgb();}
 else if(cmd==16||cmd==17||cmd==41||cmd==42){
  const unsigned count=cmd==42?3:1;
  for(unsigned pass=0;pass<count;pass++){
   const uint32_t color=cmd==42?data[20]:data[6];const auto v=vector(3);std::array<int64_t,3> base{};
   for(unsigned i=0;i<3;i++){const unsigned c=(color>>(i*8))&255;base[i]=cmd==17?int64_t(v[i])*4096:cmd==41?int64_t(c)*v[i]*16:int64_t(c)*65536;}
   depth(base,shift,positive);
  }
 }else{
  // Lighting/color commands share the documented LLM -> LCM -> color/depth pipeline.
  const bool normal=cmd==19||cmd==22||cmd==27||cmd==30||cmd==32||cmd==63;
  const bool modulate=cmd!=30&&cmd!=32;
  const bool cue=cmd==19||cmd==20||cmd==22;
  const unsigned count=(cmd==22||cmd==32||cmd==63)?3:1;
  for(unsigned n=0;n<count;n++){
   if(normal)apply(multiply(1,vector(n),3),shift,positive);
   apply(multiply(2,vector(3),1),shift,positive);
   if(modulate){std::array<int64_t,3> base{};for(unsigned i=0;i<3;i++)base[i]=int64_t((data[6]>>(i*8))&255)*sx16(data[9+i])*16;
    if(cue)depth(base,shift,positive);else{apply(base,shift,positive);push_rgb();}
   }else push_rgb();
  }
 }
 finish_flags();return true;
}
}
