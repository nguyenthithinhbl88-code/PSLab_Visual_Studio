#include "ps2_ee.hpp"
#include <algorithm>
#include <climits>
namespace ps2 {
namespace {
uint64_t sx(uint32_t x){return uint64_t(int64_t(int32_t(x)));}
uint64_t lane(Register x,unsigned i,unsigned width){return ((i*width<64?x.low:x.high)>>((i*width)&63))&((1ull<<width)-1);}
void store_lane(Register& x,unsigned i,unsigned width,uint64_t value){auto& half=i*width<64?x.low:x.high;half|=(value&((1ull<<width)-1))<<((i*width)&63);}
int64_t signed_lane(uint64_t value,unsigned width){return value&(1ull<<(width-1))?int64_t(value)-int64_t(1ull<<width):int64_t(value);}
}
bool Ee::execute_mmi(uint32_t ins){
 const unsigned s=(ins>>21)&31,t=(ins>>16)&31,d=(ins>>11)&31,sub=(ins>>6)&31,fn=ins&63;
 const auto a=r[s],b=r[t];Register result{};
 auto scalar=[&](uint64_t x){if(d)r[d].low=x;};
 switch(fn){
 case 0x10:scalar(hi.high);return true;case 0x11:hi.high=a.low;return true;
 case 0x12:scalar(lo.high);return true;case 0x13:lo.high=a.low;return true;
 case 0x18:case 0x19:{uint64_t p=fn==0x18?uint64_t(int64_t(int32_t(a.low))*int64_t(int32_t(b.low))):uint64_t(uint32_t(a.low))*uint32_t(b.low);lo.high=sx(uint32_t(p));hi.high=sx(uint32_t(p>>32));scalar(lo.high);return true;}
 case 0x1a:{const int32_t x=int32_t(a.low),y=int32_t(b.low);if(!y){lo.high=sx(x<0?1:0xffffffffu);hi.high=sx(uint32_t(x));}else if(x==INT32_MIN&&y==-1){lo.high=sx(0x80000000);hi.high=0;}else{lo.high=sx(uint32_t(x/y));hi.high=sx(uint32_t(x%y));}return true;}
 case 0x1b:{const uint32_t x=uint32_t(a.low),y=uint32_t(b.low);lo.high=sx(y?x/y:0xffffffffu);hi.high=sx(y?x%y:x);return true;}
 case 9:
  switch(sub){case 8:result=hi;break;case 9:result=lo;break;case 14:result={b.low,a.low};break;case 18:result={a.low&b.low,a.high&b.high};break;case 19:result={a.low^b.low,a.high^b.high};break;default:return false;}break;
 case 0x29:
  switch(sub){case 8:hi=a;return true;case 9:lo=a;return true;case 14:result={a.high,b.high};break;case 18:result={a.low|b.low,a.high|b.high};break;case 19:result={~(a.low|b.low),~(a.high|b.high)};break;default:return false;}break;
 case 8:case 0x28:{
  if(fn==0x28&&(sub==16||sub==17||sub==20||sub==21||sub==24||sub==25)){
   const unsigned width=32u>>((sub-16)/4);const uint64_t maximum=(1ull<<width)-1;
   for(unsigned n=0;n<128/width;n++){
    const auto x=lane(a,n,width),y=lane(b,n,width);
    const auto value=(sub&1)?(x<y?0:x-y):std::min(x+y,maximum);
    store_lane(result,n,width,value);
   }
   break;
  }
  if(fn==0x28&&sub==27){
   if(sa>15)return false;
   const uint64_t words[]={b.low,b.high,a.low,a.high};
   for(unsigned n=0;n<16;n++){unsigned at=n+sa;store_lane(result,n,8,(words[at/8]>>((at%8)*8))&255);}
   break;
  }
  // Modular packed arithmetic/comparison, independently in every 8/16/32-bit lane.
  const bool equal=fn==0x28;
  if((equal&&(sub!=2&&sub!=6&&sub!=10))||(!equal&&sub>10))return false;
  const unsigned width=sub<4?32:sub<8?16:8,operation=sub&3;
  if(!equal&&width==8&&operation==3)return false;
  for(unsigned n=0;n<128/width;n++){
   auto x=lane(a,n,width),y=lane(b,n,width);uint64_t value=0;
   if(equal)value=x==y?((1ull<<width)-1):0;
   else if(operation==0)value=x+y;
   else if(operation==1)value=x-y;
   else if(operation==2)value=signed_lane(x,width)>signed_lane(y,width)?((1ull<<width)-1):0;
   else value=signed_lane(x,width)>signed_lane(y,width)?x:y;
   store_lane(result,n,width,value);
  }break;
 }
 default:return false;
 }
 if(d)r[d]=result;
 return true;
}
}
