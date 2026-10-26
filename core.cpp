#include "core.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iomanip>
namespace ps1 {
static std::runtime_error bus_error(const char* operation,uint32_t a){std::ostringstream s;s<<operation<<" @ 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<a;return std::runtime_error(s.str());}
static uint32_t physical(uint32_t a) { return a & 0x1fffffff; }
static bool timer_address(uint32_t a){a=physical(a);return a>=0x1f801100&&a<0x1f801130&&((a&15)==0||(a&15)==4||(a&15)==8);}
void Bus::sample_irq_lines(){
 uint16_t lines=(gpu.status&(1u<<24))?2:0;
 if(cd.irq())lines|=4;
 if(spu.irq)lines|=1u<<9;
 for(unsigned i=0;i<2;i++)if(sio[i].irq)lines|=uint16_t(1u<<(7+i));
 irq_status|=uint16_t(lines&~previous_irq_lines);previous_irq_lines=lines;
}
void Bus::tick_timers(){
 cd.clocks++;
 for(auto& port:sio)if(port.ack_delay||port.ack_width)port.tick();
 for(unsigned ch:{2u,3u,4u})if(dma[ch].chop_wait)dma[ch].chop_wait--;
 const unsigned sources=((timers[0].mode&0x100)?1:0)|(((timers[0].mode&1)||(timers[1].mode&0x100))?2:0);
 const bool vblank=gpu.tick_video(sources);if(vblank)irq_status|=1;
 sample_irq_lines();
 if(cd.audio_start_delay||cd.irq_delay||cd.response_delay||cd.busy||cd.reading||!cd.events.empty()||!cd.pending.empty())cd.tick();
 if(dma[3].control&(1u<<24))run_dma(3);
 if(dma[0].control&(1u<<24))run_dma(0);
 if(dma[1].control&(1u<<24))run_dma(1);
 for(unsigned ch:{2u,4u})if((dma[ch].control&0x1000100u)==0x1000100u)run_dma(ch);
 if(++spu.sample_clock==768){spu.sample_clock=0;const auto pcm=cd.audio_sample();spu.render_sample(pcm[0],pcm[1]);}sample_irq_lines();
 if(timers[0].tick(0,gpu.timer_dots,gpu.video_hblank,gpu.timer_hblank_rise))irq_status|=1u<<4;
 if(timers[1].tick(1,gpu.timer_hblank_rise?1:0,gpu.video_vblank,vblank))irq_status|=1u<<5;
 if(timers[2].tick(2))irq_status|=1u<<6;
}

uint8_t Bus::read8(uint32_t a) const {
    const uint32_t fast=physical(a);
    if(fast<0x800000)return ram[fast&0x1fffff];
    if(fast>=0x1f800000&&fast<0x1f800400)return scratch[fast-0x1f800000];
    if(fast>=0x1fc00000&&fast<0x1fc80000)return bios[fast-0x1fc00000];
    if(physical(a)>=0x1f801040&&physical(a)<0x1f801060)return sio[(physical(a)-0x1f801040)/16].read(a&15);
    if(physical(a)>=0x1f801800&&physical(a)<=0x1f801803)return cd.read(physical(a)-0x1f801800);
    // DMA registers are side-effect-free on reads at any supported CPU width.
    // Route each byte lane through the same register decoder used by LW.
    uint32_t dma_value=0;
    const auto p=physical(a);
    if(p>=0x1f801080&&p<0x1f8010f8&&dma_read(p&~3u,dma_value))return uint8_t(dma_value>>((p&3)*8));
    if(p>=0x1f801c00&&p<0x1f801e80)return uint8_t(spu.read((p-0x1f801c00)&~1u)>>((p&1)*8));
    if(p==0x1f801814||p==0x1f801815||p==0x1f801816||p==0x1f801817)return uint8_t(gpu.status>>((p&3)*8));
    if(p>=0x1f801824&&p<0x1f801828)return uint8_t(mdec.status()>>((p&3)*8));
    uint8_t v=0;if(boot_io_read(a,v))return v;
    const uint32_t original=a;a=physical(a);
    if(a<0x800000) return ram[a&0x1fffff];
    if(a>=0x1f800000 && a<0x1f800400) return scratch[a-0x1f800000];
    if(a>=0x1fc00000 && a<0x1fc80000) return bios[a-0x1fc00000];
    throw bus_error("Unsupported bus read",original);
}
uint16_t Bus::read16(uint32_t a)const{
 const uint32_t fast=physical(a);
 if(!(a&1)&&fast<0x800000){unsigned i=fast&0x1fffff;return uint16_t(ram[i]|(uint16_t(ram[i+1])<<8));}
 if(!(a&1)&&fast>=0x1f800000&&fast<0x1f800400){unsigned i=fast-0x1f800000;return uint16_t(scratch[i]|(uint16_t(scratch[i+1])<<8));}
 if(!(a&1)&&fast>=0x1fc00000&&fast<0x1fc80000){unsigned i=fast-0x1fc00000;return uint16_t(bios[i]|(uint16_t(bios[i+1])<<8));}
 if(timer_address(a))return timers[(physical(a)-0x1f801100)/16].read(a&15);
 if(a&1)throw bus_error("Unaligned halfword read",a);
 uint32_t p=physical(a);
 if(p>=0x1f801800&&p<=0x1f801802){const unsigned port=p-0x1f801800;const bool increment=memory_control[6]&0x2000;const uint16_t low=cd.read(port);return uint16_t(low|(uint16_t(cd.read(increment?port+1:port))<<8));}
 if(p>=0x1f801c00&&p<0x1f801e80)return spu.read(p-0x1f801c00);
 return uint16_t(read8(a)|(uint16_t(read8(a+1))<<8));
}
void Bus::write16(uint32_t a,uint16_t v){
 const uint32_t fast=physical(a);
 if(!(a&1)&&fast<0x800000){unsigned i=fast&0x1fffff;ram[i]=uint8_t(v);ram[i+1]=uint8_t(v>>8);return;}
 if(!(a&1)&&fast>=0x1f800000&&fast<0x1f800400){unsigned i=fast-0x1f800000;scratch[i]=uint8_t(v);scratch[i+1]=uint8_t(v>>8);return;}
 if(timer_address(a)){timers[(physical(a)-0x1f801100)/16].write(a&15,v);return;}
 if(a&1)throw bus_error("Unaligned halfword write",a);
 uint32_t p=physical(a);
 if(dma_subword_write(a,v,2))return;
 if(p>=0x1f801800&&p<=0x1f801802){const unsigned port=p-0x1f801800;const bool increment=memory_control[6]&0x2000;cd.write(port,uint8_t(v));cd.write(increment?port+1:port,uint8_t(v>>8));return;}
 if(p>=0x1f801c00&&p<0x1f801e80){spu.write(p-0x1f801c00,v);return;}
 write8(a,uint8_t(v));write8(a+1,uint8_t(v>>8));
}
uint32_t Bus::read32(uint32_t a) const {
    // CPU instruction fetches and ordinary RAM loads avoid four MMIO decodes.
    const uint32_t p=physical(a);
    if(!(a&3)&&p<0x800000){const auto i=p&0x1fffff;return uint32_t(ram[i])|(uint32_t(ram[i+1])<<8)|(uint32_t(ram[i+2])<<16)|(uint32_t(ram[i+3])<<24);}
    if(!(a&3)&&p>=0x1fc00000&&p<0x1fc80000){const auto i=p-0x1fc00000;return uint32_t(bios[i])|(uint32_t(bios[i+1])<<8)|(uint32_t(bios[i+2])<<16)|(uint32_t(bios[i+3])<<24);}
    if(!(a&3)&&p==0x1f801800){uint32_t value=0;const bool increment=memory_control[6]&0x2000;for(unsigned i=0;i<4;i++)value|=uint32_t(cd.read(increment?i:0))<<(i*8);return value;}
    if(timer_address(a))return read16(a);
    if(!(a&3)&&physical(a)>=0x1f801c00&&physical(a)<0x1f801e80)return uint32_t(read16(a))|(uint32_t(read16(a+2))<<16);
    uint32_t dma_value=0;if(dma_read(physical(a),dma_value))return dma_value;
    if(p==0x1f801820)return mdec.read();
    if(p==0x1f801824)return mdec.status();
    if(physical(a)==0x1f801810)return gpu.read_gp0();
    if(physical(a)==0x1f801814)return gpu.status;
    if(a&3) throw std::runtime_error("Unaligned word read");
    uint32_t v=0; for(unsigned i=0;i<4;i++) v|=uint32_t(read8(a+i))<<(8*i); return v;
}
void Bus::write8(uint32_t a,uint8_t v) {
    const uint32_t fast=physical(a);
    if(fast<0x800000){ram[fast&0x1fffff]=v;return;}
    if(fast>=0x1f800000&&fast<0x1f800400){scratch[fast-0x1f800000]=v;return;}
    if(physical(a)>=0x1f801040&&physical(a)<0x1f801060){sio[(physical(a)-0x1f801040)/16].write(a&15,v);return;}
    if(physical(a)>=0x1f801800&&physical(a)<=0x1f801803){cd.write(physical(a)-0x1f801800,v);return;}
    if(dma_subword_write(a,v,1))return;
    if(physical(a)>=0x1f801c00&&physical(a)<0x1f801e80){if(!(a&1))spu.write(physical(a)-0x1f801c00,v);return;}
    if(physical(a)>=0x1f801e80&&physical(a)<0x1f802000)return;
    if(boot_io_write(a,v))return;
    const uint32_t original=a;a=physical(a);
    if(a<0x800000) {ram[a&0x1fffff]=v;return;}
    if(a>=0x1f800000 && a<0x1f800400) {scratch[a-0x1f800000]=v;return;}
    throw bus_error("Unsupported/read-only bus write",original);
}
void Bus::write32(uint32_t a,uint32_t v) {
    const uint32_t p=physical(a);
    if(!(a&3)&&p<0x800000){const auto i=p&0x1fffff;for(unsigned byte=0;byte<4;byte++)ram[i+byte]=uint8_t(v>>(byte*8));return;}
    if(!(a&3)&&p==0x1f801800){const bool increment=memory_control[6]&0x2000;for(unsigned i=0;i<4;i++)cd.write(increment?i:0,uint8_t(v>>(i*8)));return;}
    if(timer_address(a)){write16(a,uint16_t(v));return;}
    if(!(a&3)&&physical(a)>=0x1f801c00&&physical(a)<0x1f801e80){write16(a,uint16_t(v));write16(a+2,uint16_t(v>>16));return;}
    if(p>=0x1f8010f8&&p<0x1f801100)return;
    if(p>=0x1f801e80&&p<0x1f802000)return;
    if(dma_write(physical(a),v))return;
    if(p==0x1f801820){mdec.write(v);return;}
    if(p==0x1f801824){mdec.write_control(v);return;}
    if(physical(a)==0x1f801810){gpu.gp0(v);return;}
    if(physical(a)==0x1f801814){gpu.gp1(v);return;}
    if(a&3) throw std::runtime_error("Unaligned word write");
    for(unsigned i=0;i<4;i++) write8(a+i,uint8_t(v>>(8*i)));
}
void Bus::load_bios(const std::string& path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f || f.tellg()!=std::streamoff(bios.size())) throw std::runtime_error("BIOS must be exactly 512 KiB");
    f.seekg(0); if(!f.read(reinterpret_cast<char*>(bios.data()),bios.size())) throw std::runtime_error("BIOS read failed");
}
static int64_t signed32(uint32_t v) { return v<0x80000000u ? int64_t(v) : int64_t(v)-0x100000000ll; }
static uint32_t sar(uint32_t v,unsigned n) {
    n&=31; if(!n) return v;
    return (v>>n) | ((v&0x80000000u) ? (~uint32_t(0)<<(32-n)) : 0);
}
void Cpu::reset(uint32_t entry) {
    gte=Gte{};first_fault=FaultSnapshot{};bios_system_error=false;
    bios_calls=0;last_bios_vector=last_bios_function=last_bios_return=0;last_bios_args.fill(0);
    instructions=interrupts=reserved_exceptions=0;last_reserved_pc=last_reserved_opcode=0;trace.fill({});r.fill(0);cop0.fill(0);cop0[12]=0x00400000;cop0[15]=2;
    hi=lo=0;pc=entry;next_pc=entry+4;pending_reg=0;pending_value=0;delay_slot=false;
}
void Cpu::step() {
    const uint32_t ip=pc;
    const bool in_delay=delay_slot;
    uint32_t fetched_opcode=0;bool has_opcode=false;
    auto exception=[&](unsigned code,unsigned coprocessor=0) {
        const uint32_t previous_status=cop0[12];
        if(code==10){reserved_exceptions++;last_reserved_pc=ip;last_reserved_opcode=fetched_opcode;}
        cop0[13]=(cop0[13]&0x0000ff00u)|(code<<2)|(code==11?(coprocessor<<28):0)|(in_delay?0x80000000u:0);
        cop0[14]=in_delay?ip-4:ip;
        cop0[12]=(cop0[12]&~0x3fu)|((cop0[12]<<2)&0x3f);
        if(code!=0&&code!=8&&!first_fault.valid){
            first_fault.valid=true;first_fault.has_opcode=has_opcode;first_fault.in_delay=in_delay;
            first_fault.code=code;first_fault.pc=ip;first_fault.opcode=fetched_opcode;
            first_fault.status=previous_status;first_fault.cause=cop0[13];first_fault.epc=cop0[14];first_fault.instruction=instructions;
            first_fault.registers=r;first_fault.trace=trace;first_fault.gte_data=gte.data;first_fault.gte_control=gte.control;
        }
        pc=(cop0[12]&0x400000)?0xbfc00180:0x80000080;next_pc=pc+4;delay_slot=false;
    };
    cop0[13]=(cop0[13]&~0x400u)|((bus.irq_status&bus.irq_mask)?0x400u:0);
    if((cop0[12]&1)&&(cop0[12]&cop0[13]&0xff00)){
        if(pending_reg)r[pending_reg]=pending_value;
        pending_reg=0;exception(0);interrupts++;return;
    }
    if((ip&3)||((cop0[12]&2)&&(ip&0x80000000u))){ if(pending_reg)r[pending_reg]=pending_value;pending_reg=0;cop0[8]=ip;exception(4);return; }
    const uint32_t vector=physical(ip);
    if(vector>=0xa0&&vector<=0xc0&&(vector&15)==0){
        bios_calls++;last_bios_vector=vector;last_bios_function=r[9];last_bios_return=r[31];
        for(unsigned i=0;i<4;i++)last_bios_args[i]=r[4+i];
        if(vector==0xa0&&r[9]==0x40&&!bus.direct_exe_boot)bios_system_error=true;
        if(bus.direct_exe_boot&&bus.read32(vector)==0&&bus.read32(vector+4)==0&&bus.read32(vector+8)==0&&bus.read32(vector+12)==0){
            std::ostringstream message;message<<"Direct EXE called BIOS "<<std::hex<<vector<<"("<<r[9]<<"), but kernel RAM vector is empty. Nap BIOS -> Boot Game CD -> Run.";
            throw std::runtime_error(message.str());
        }
    }
    // Fetch RAM/BIOS directly after the architectural alignment/IRQ checks.
    // Other address regions retain the bus decoder and its explicit faults.
    auto word=[](const uint8_t* data){return uint32_t(data[0])|(uint32_t(data[1])<<8)|(uint32_t(data[2])<<16)|(uint32_t(data[3])<<24);};
    const uint32_t ins=vector<0x800000?word(bus.ram.data()+(vector&0x1fffff)):
        vector>=0x1fc00000&&vector<0x1fc80000?word(bus.bios.data()+vector-0x1fc00000):bus.read32(ip);
    fetched_opcode=ins;has_opcode=true;
    trace[instructions%trace.size()]={ip,ins,r[(ins>>21)&31],r[(ins>>16)&31],pending_reg,pending_value};instructions++;
    // Observe BIOS putchar entry without replacing or skipping its instructions.
    if(((ip&0x1fffffff)==0xa0 && r[9]==0x3c)||((ip&0x1fffffff)==0xb0 && r[9]==0x3d))bus.append_tty(char(r[4]&255));
    const unsigned op=ins>>26,s=(ins>>21)&31,t=(ins>>16)&31,d=(ins>>11)&31,sh=(ins>>6)&31;
    const uint32_t a=r[s],b=r[t],imm=uint32_t(int32_t(int16_t(ins&65535)));
    const unsigned old_pending=pending_reg;const uint32_t old_value=pending_value;
    if(pending_reg)r[pending_reg]=pending_value;
    pending_reg=0;pc=next_pc;next_pc=pc+4;delay_slot=false;
    auto put=[&](unsigned reg,uint32_t value){if(reg)r[reg]=value;};
    auto load=[&](uint32_t value){
        // A second load to the same destination cancels the older writeback.
        // Its operand reads still use the pre-load GPR values captured above.
        if(t&&old_pending==t)r[t]=b;
        pending_reg=t;pending_value=value;
    };
    auto branch=[&](bool take){delay_slot=true;if(take)next_pc=ip+4+(imm<<2);};
    auto arithmetic=[&](unsigned reg,int64_t value){if(value>2147483647ll||value< -2147483648ll)exception(12);else put(reg,uint32_t(value));};
    const uint32_t address=a+imm;
    auto store=[&](unsigned width){
        const auto p=physical(address);
        if(bus.dma_subword_write(address,b,width))return;
        if(width==1&&p>=0x1f801c00&&p<0x1f801e80){if(!(p&1))bus.spu.write(p-0x1f801c00,uint16_t(b));return;}
        if(p==0x1f801820||p==0x1f801824){bus.write32(address,b);return;}
        if(width==1)bus.write8(address,uint8_t(b));else bus.write16(address,uint16_t(b));
    };
    auto aligned=[&](unsigned mask,bool store){if(address&mask){cop0[8]=address;exception(store?5:4);return false;}return true;};
    auto half=[&](){return uint32_t(bus.read16(address));};
    // KUc applies even to byte/unaligned transfers and isolated-cache stores.
    const bool memory_load=(op>=32&&op<=38)||op==50;
    const bool memory_store=(op>=40&&op<=43)||op==46||op==58;
    if((cop0[12]&2)&&(address&0x80000000u)&&(memory_load||memory_store)){
        // COP2 permission is checked before its memory address.
        if((op==50||op==58)&&!(cop0[12]&(1u<<30)))exception(11,2);
        else{cop0[8]=address;exception(memory_store?5:4);}
        r[0]=0;bus.tick_timers();return;
    }
    switch(op) {
    case 0:
        switch(ins&63) {
        case 0:put(d,b<<sh);break;case 2:put(d,b>>sh);break;case 3:put(d,sar(b,sh));break;
        case 4:put(d,b<<(a&31));break;case 6:put(d,b>>(a&31));break;case 7:put(d,sar(b,a));break;
        case 8:delay_slot=true;next_pc=a;break;
        case 9:put(d,ip+8);delay_slot=true;next_pc=a;break;
        case 12:exception(8);break;case 13:exception(9);break;
        case 16:put(d,hi);break;case 17:hi=a;break;case 18:put(d,lo);break;case 19:lo=a;break;
        case 24:{uint64_t v=uint64_t(signed32(a)*signed32(b));lo=uint32_t(v);hi=uint32_t(v>>32);break;}
        case 25:{uint64_t v=uint64_t(a)*b;lo=uint32_t(v);hi=uint32_t(v>>32);break;}
        case 26:if(!b){lo=signed32(a)<0?1:0xffffffff;hi=a;}else{lo=uint32_t(signed32(a)/signed32(b));hi=uint32_t(signed32(a)%signed32(b));}break;
        case 27:if(!b){lo=0xffffffff;hi=a;}else{lo=a/b;hi=a%b;}break;
        case 32:arithmetic(d,signed32(a)+signed32(b));break;
        case 33:put(d,a+b);break;case 34:arithmetic(d,signed32(a)-signed32(b));break;case 35:put(d,a-b);break;
        case 36:put(d,a&b);break;case 37:put(d,a|b);break;case 38:put(d,a^b);break;case 39:put(d,~(a|b));break;
        case 42:put(d,signed32(a)<signed32(b));break;case 43:put(d,a<b);break;
        default:exception(10);break;
        }break;
    case 1:
        if(t==0||t==1||t==16||t==17){if(t&16)put(31,ip+8);branch((t&1)?signed32(a)>=0:signed32(a)<0);}else exception(10);break;
    case 2:case 3:if(op==3)put(31,ip+8);delay_slot=true;next_pc=((ip+4)&0xf0000000)|((ins&0x3ffffff)<<2);break;
    case 4:branch(a==b);break;case 5:branch(a!=b);break;
    case 6:branch(signed32(a)<=0);break;case 7:branch(signed32(a)>0);break;
    case 8:arithmetic(t,signed32(a)+signed32(imm));break;case 9:put(t,a+imm);break;
    case 10:put(t,signed32(a)<signed32(imm));break;case 11:put(t,a<imm);break;
    case 12:put(t,a&(ins&65535));break;case 13:put(t,a|(ins&65535));break;case 14:put(t,a^(ins&65535));break;case 15:put(t,(ins&65535)<<16);break;
    case 16:
        if((cop0[12]&2)&&!(cop0[12]&(1u<<28))&&!(s==0&&d>=16)){exception(11,0);break;}
        if(s==0){if(d==0||d==1||d==2||d==4||d==10)exception(10);else load(cop0[d]);}
        else if(s==4){
            if(d==12||d==14||d==3||d==5||d==6||d==7||d==9||d==11)cop0[d]=b;
            else if(d==13)cop0[13]=(cop0[13]&~0x300u)|(b&0x300);
        }else if(s==16&&(ins&63)==16)cop0[12]=(cop0[12]&~15u)|((cop0[12]>>2)&15);
        else exception(10);
        break;
    case 18:
        if(!(cop0[12]&(1u<<30))){exception(11,2);break;}
        if(s==0)load(gte.read_data(d));
        else if(s==2)load(gte.read_control(d));
        else if(s==4)gte.write_data(d,b);
        else if(s==6)gte.write_control(d,b);
        else if(s==8&&(t==0||t==1))branch(t==0); // BC2F always; BC2T never
        else if(s>=16){if(!gte.execute(ins))exception(10);}
        else exception(10);
        break;
    case 50:
        if(!(cop0[12]&(1u<<30)))exception(11,2);
        else if(aligned(3,false))gte.write_data(t,bus.read32(address));
        break;
    case 58:
        if(!(cop0[12]&(1u<<30)))exception(11,2);
        else if(aligned(3,true)&&!(cop0[12]&0x10000))bus.write32(address,gte.read_data(t));
        break;
    case 48:case 56:exception(11,0);break;
    case 17:case 19:case 49:case 51:case 57:case 59:exception(11,op&3);break;
    case 32:load(uint32_t(int32_t(int8_t(bus.read8(address)))));break;
    case 33:if(aligned(1,false))load(uint32_t(int32_t(int16_t(half()))));break;
    case 35:if(aligned(3,false))load(bus.read32(address));break;
    case 36:load(bus.read8(address));break;case 37:if(aligned(1,false))load(half());break;
    case 34:case 38:{
        const uint32_t word=bus.read32(address&~3u),previous=(old_pending==t&&t)?old_value:b;
        const unsigned n=address&3;
        if(op==34){unsigned shift=(3-n)*8;load((previous&(0x00ffffffu>>(n*8)))|(word<<shift));}
        else {unsigned shift=n*8;uint32_t mask=n?0xffffffffu<<(32-shift):0;load((previous&mask)|(word>>shift));}break;}
    case 40:if(!(cop0[12]&0x10000))store(1);break;
    case 41:if(aligned(1,true)&&!(cop0[12]&0x10000)){store(2);}break;
    case 43:if(aligned(3,true)&&!(cop0[12]&0x10000))bus.write32(address,b);break;
    case 42:case 46:if(!(cop0[12]&0x10000)){
        uint32_t word=bus.read32(address&~3u);unsigned n=address&3;
        if(op==42){unsigned shift=(3-n)*8;uint32_t mask=n==3?0:0xffffffffu<<((n+1)*8);word=(word&mask)|(b>>shift);}
        else {unsigned shift=n*8;uint32_t mask=n?(1u<<shift)-1:0;word=(word&mask)|(b<<shift);}
        bus.write32(address&~3u,word);
    }break;
    default:exception(10);break;
    }
    r[0]=0;bus.tick_timers();
}
std::string Cpu::failure_summary()const{
 std::ostringstream out;out<<"BIOS SystemError A0(40)";
 if(first_fault.valid)out<<" after CPU exception "<<std::dec<<first_fault.code<<" at 0x"<<std::hex<<first_fault.pc<<" opcode=0x"<<first_fault.opcode;
 out<<". Save log includes the first fault snapshot.";return out.str();
}
std::string Cpu::diagnostic()const{
 std::ostringstream out;out<<"PSLab v31.1 PS1 diagnostic\nInstructions: "<<instructions<<"\n";
 out<<std::hex<<std::setfill('0');
 out<<"PC="<<std::setw(8)<<pc<<" Next="<<std::setw(8)<<next_pc<<" SR="<<std::setw(8)<<cop0[12]<<" Cause="<<std::setw(8)<<cop0[13]<<" EPC="<<std::setw(8)<<cop0[14]<<"\n";
 if(first_fault.valid){
  out<<"FIRST CPU FAULT code="<<std::dec<<first_fault.code<<" instruction="<<first_fault.instruction<<" PC="<<std::hex<<first_fault.pc<<" opcode="<<first_fault.opcode<<" fetched="<<first_fault.has_opcode<<" delay="<<first_fault.in_delay<<" SR="<<first_fault.status<<" Cause="<<first_fault.cause<<" EPC="<<first_fault.epc<<"\n";
  for(unsigned i=0;i<32;i++)out<<"F_R"<<std::dec<<i<<"="<<std::hex<<std::setw(8)<<first_fault.registers[i]<<((i%4==3)?"\n":" ");
  out<<"GTE registers at first fault:\n";for(unsigned i=0;i<32;i++)out<<"D"<<std::dec<<i<<"="<<std::hex<<first_fault.gte_data[i]<<" C"<<std::dec<<i<<"="<<std::hex<<first_fault.gte_control[i]<<"\n";
  out<<"Trace saved at first fault (oldest first):\n";
  const uint64_t first=first_fault.instruction>trace.size()?first_fault.instruction-trace.size():0;
  for(uint64_t i=first;i<first_fault.instruction;i++){const auto& item=first_fault.trace[i%trace.size()];out<<std::hex<<std::setw(8)<<item.pc<<": "<<std::setw(8)<<item.opcode<<"\n";}
 }
 out<<"GTE commands="<<std::dec<<gte.commands<<" last="<<std::hex<<gte.last_command<<" FLAG="<<gte.read_control(31)<<" BIOS_SystemError="<<bios_system_error<<"\n";
 out<<"Boot mode="<<(bus.direct_exe_boot?"direct-exe (kernel not initialized by loader)":"machine/BIOS path")<<" BIOS reset word="<<bus.read32(0xbfc00000)<<"\n";
 out<<"BIOS calls="<<std::dec<<bios_calls<<" last_vector="<<std::hex<<last_bios_vector<<" function="<<last_bios_function<<" return="<<last_bios_return<<" args=";for(auto value:last_bios_args)out<<value<<" ";out<<"\n";
 out<<"RAM vectors A0/B0/C0:";for(unsigned vector=0xa0;vector<=0xc0;vector+=16){out<<" ["<<std::hex<<vector<<"]";for(unsigned i=0;i<16;i+=4)out<<" "<<bus.read32(vector+i);}out<<"\n";
 out<<"POST="<<unsigned(bus.post)<<" RAM control="<<bus.ram_control<<" Cache control="<<bus.cache_control<<"\n";
 for(unsigned i=0;i<32;i++)out<<"R"<<std::dec<<i<<"="<<std::hex<<std::setw(8)<<r[i]<<((i%4==3)?"\n":" ");
 out<<"Recent fetched instructions (oldest first):\n";
 uint64_t start=instructions>trace.size()?instructions-trace.size():0;
 for(uint64_t i=start;i<instructions;i++){const auto& t=trace[i%trace.size()];out<<std::setw(8)<<t.pc<<": "<<std::setw(8)<<t.opcode<<" rs="<<t.rs_value<<" rt="<<t.rt_value<<" pending="<<t.load_reg<<":"<<t.load_value<<"\n";}
 for(unsigned i=0;i<3;i++)out<<"Timer"<<i<<" count="<<bus.timers[i].count<<" mode="<<bus.timers[i].mode<<" target="<<bus.timers[i].target<<"\n";
 out<<"CPU IRQ exceptions="<<std::dec<<interrupts<<" reserved_exceptions="<<reserved_exceptions<<" last_reserved_pc="<<std::hex<<last_reserved_pc<<" last_reserved_opcode="<<last_reserved_opcode<<"\n";
 out<<"DPCR="<<std::hex<<bus.dpcr<<" DICR="<<bus.dicr<<" I_STAT="<<bus.irq_status<<" I_MASK="<<bus.irq_mask<<"\n";
 for(unsigned ch=0;ch<7;ch++){const auto& d=bus.dma[ch];if(d.chop_bursts||d.chop_wait)out<<"DMA chopping channel="<<ch<<" control="<<std::hex<<d.control<<" address="<<d.address<<" block="<<d.block<<" remaining="<<std::dec<<d.remaining<<" cpu_wait="<<d.chop_wait<<" bursts="<<d.chop_bursts<<"\n";}
 out<<"MDEC status="<<std::hex<<bus.mdec.status()<<" control="<<bus.mdec.control<<" command="<<bus.mdec.command<<" remaining="<<std::dec<<bus.mdec.remaining<<" input_halfwords="<<bus.mdec.input.size()<<" output_words="<<bus.mdec.output.size()<<" commands="<<bus.mdec.commands<<" decoded_macroblocks="<<bus.mdec.macroblocks<<" words_in="<<bus.mdec.words_in<<" words_out="<<bus.mdec.words_out<<" resets="<<bus.mdec.resets<<"\n";
 for(unsigned i=0;i<2;i++){const auto& d=bus.dma[i];out<<"MDEC DMA"<<i<<" address="<<std::hex<<d.address<<" block="<<d.block<<" control="<<d.control<<" remaining="<<std::dec<<d.remaining<<"\n";}
 out<<"CD sectors="<<std::dec<<bus.cd.sectors<<" lba="<<bus.cd.lba<<" target="<<bus.cd.target_lba<<" setloc_pending="<<bus.cd.setloc_pending<<" commands="<<bus.cd.commands<<" sectors_read="<<bus.cd.sectors_read<<" irq="<<unsigned(bus.cd.flag)<<"\n";
 out<<"Disc layout: "<<bus.cd.disc.description()<<"\n";
 out<<"CD mode="<<std::hex<<unsigned(bus.cd.mode)<<" last_command="<<unsigned(bus.cd.last_command)<<" error="<<unsigned(bus.cd.last_error)<<" data_fifo="<<std::dec<<bus.cd.data.size()<<" pending="<<bus.cd.pending.size()<<" region_emulated="<<bus.cd.effective_region()<<" boot="<<bus.cd.boot_name<<"\n";
 out<<"CD audio: playing="<<bus.cd.playing<<" track="<<bus.cd.playing_track<<" sectors="<<std::dec<<bus.cd.cdda_sectors<<" frames="<<bus.cd.cdda_frames<<" muted="<<bus.cd.muted<<" ATV=";for(auto v:bus.cd.active_volume)out<<std::hex<<unsigned(v)<<" ";out<<"\n";
 out<<"XA audio: sectors="<<std::dec<<bus.cd.xa.sectors<<" frames="<<bus.cd.xa.frames<<" queued="<<bus.cd.xa.pcm.size()<<" dropped="<<bus.cd.xa.dropped<<" invalid_coding="<<bus.cd.xa.invalid<<"\n";
 out<<"SPU frames="<<std::dec<<bus.spu.generated_frames<<" queued_frames="<<bus.spu.audio.size()/2<<" dropped_frames="<<bus.spu.dropped_frames<<" active_voices="<<bus.spu.active_voices()<<" irq9="<<bus.spu.irq<<" capture="<<std::hex<<bus.spu.capture_position<<" CNT="<<bus.spu.read(0x1aa)<<" STAT="<<bus.spu.read(0x1ae)<<"\n";
 for(unsigned i=0;i<24;i++){const auto& v=bus.spu.voices[i];if(v.phase!=Spu::Voice::Off)out<<"SPU voice"<<std::dec<<i<<" phase="<<v.phase<<" level="<<v.level<<" address="<<std::hex<<v.address<<" pitch="<<bus.spu.regs[i*8+2]<<"\n";}
 out<<"CD controller: index="<<std::dec<<unsigned(bus.cd.index)<<" enable="<<std::hex<<unsigned(bus.cd.enable)<<" flag="<<unsigned(bus.cd.flag)<<" busy="<<bus.cd.busy<<" status="<<unsigned(bus.cd.status())<<" irq_ack_delay="<<std::dec<<bus.cd.irq_delay<<" sector_buffer="<<std::dec<<bus.cd.sector_buffer.size()<<" sector_pending="<<bus.cd.sector_pending<<" sector_delay="<<bus.cd.sector_delay<<" response_delay="<<bus.cd.response_delay<<" response_fifo="<<std::dec<<bus.cd.response.size()<<" parameter_fifo="<<bus.cd.params.size()<<" queued_events="<<bus.cd.events.size()<<" queued_commands="<<bus.cd.pending.size()<<" DEV5="<<std::hex<<bus.memory_control[6]<<"\n";
 out<<"CD protocol history (oldest first):\n";
 const uint64_t first_cd=bus.cd.transactions>bus.cd.history.size()?bus.cd.transactions-bus.cd.history.size():0;
 for(uint64_t i=first_cd;i<bus.cd.transactions;i++){
  const auto& item=bus.cd.history[i%bus.cd.history.size()];out<<std::dec<<i<<" clock="<<item.clock<<" "<<item.event<<" value="<<std::hex<<unsigned(item.value)<<" index="<<unsigned(item.index)<<" irq="<<unsigned(item.flag)<<" enable="<<unsigned(item.enable)<<" status="<<unsigned(item.status)<<" lba="<<std::dec<<item.lba<<" bytes="<<std::hex;
  for(unsigned j=0;j<item.size;j++)out<<unsigned(item.bytes[j])<<" ";
  out<<"\n";
 }
 out<<"CD DMA history (oldest first, checksum=FNV1a):\n";
 const uint64_t first_dma=bus.cd_dma_transfers>bus.cd_dma_history.size()?bus.cd_dma_transfers-bus.cd_dma_history.size():0;
 for(uint64_t i=first_dma;i<bus.cd_dma_transfers;i++){
  const auto& d=bus.cd_dma_history[i%bus.cd_dma_history.size()];out<<std::dec<<i<<" clock="<<d.clock<<" lba="<<d.lba<<" address="<<std::hex<<d.address<<" words="<<std::dec<<d.words<<" checksum="<<std::hex<<d.checksum<<" first_words=";
  for(auto word:d.sample){out<<word<<" ";}out<<"\n";
 }
 auto ram_window=[&](uint32_t address){
  const uint32_t base=physical(address)&~15u;if(base>=0x800000)return;
  out<<"RAM window "<<std::hex<<address<<" (live RAM, no MMIO):\n";
  for(unsigned offset=0;offset<128;offset+=16){const auto at=base+offset;if(at>=0x800000)break;out<<std::setw(8)<<at<<":";for(unsigned lane=0;lane<16;lane+=4)out<<" "<<std::setw(8)<<bus.read32(at+lane);out<<"\n";}
 };
 ram_window(0x10000);if((physical(r[4])&~15u)!=0x10000)ram_window(r[4]);
 out<<"Pad connected="<<bus.sio[0].connected<<" buttons="<<std::hex<<bus.sio[0].buttons<<" phase="<<std::dec<<bus.sio[0].phase<<" polls="<<bus.sio[0].polls<<" ACK_events="<<bus.sio[0].ack_events<<" ACK_delay="<<bus.sio[0].ack_delay<<" ACK_level="<<bus.sio[0].ack<<"\n";
 out<<"Pad protocol transfers="<<std::dec<<bus.sio[0].transfers<<" complete="<<bus.sio[0].completed_polls<<" pressed="<<bus.sio[0].pressed_polls<<" latched="<<std::hex<<bus.sio[0].latched_buttons<<" last_pressed="<<bus.sio[0].last_pressed_buttons<<" CTRL="<<bus.sio[0].control<<" MODE="<<bus.sio[0].mode<<" BAUD="<<bus.sio[0].baud<<" IRQ="<<bus.sio[0].irq<<" last_TX="<<unsigned(bus.sio[0].last_tx)<<"\n";
 out<<"GPUSTAT="<<std::hex<<bus.gpu.status<<" DrawMode="<<bus.gpu.draw_mode<<" TextureWindow="<<bus.gpu.texture_window<<" PacketWords="<<std::dec<<bus.gpu.count<<"\n";
 out<<"GPU words="<<bus.gpu.words<<" polygons="<<bus.gpu.polygons<<" rectangles="<<bus.gpu.rectangles<<" lines="<<bus.gpu.line_segments<<"\n";
 out<<"Video line="<<std::dec<<bus.gpu.video_line<<" dot="<<bus.gpu.video_dot<<" field="<<bus.gpu.video_field<<" vblank="<<bus.gpu.video_vblank<<" frames="<<bus.gpu.video_frames<<" vblank_events="<<bus.gpu.vblank_events<<"\n";
 out<<"GPU display registers:";for(auto word:bus.gpu.display)out<<" "<<std::hex<<word;out<<"\n";
 out<<"Recent GP0 words (oldest first):\n";
 const auto first=bus.gpu.words>bus.gpu.history.size()?bus.gpu.words-bus.gpu.history.size():0;
 for(uint64_t i=first;i<bus.gpu.words;i++)out<<std::hex<<std::setw(8)<<bus.gpu.history[i%bus.gpu.history.size()]<<((i%8==7)?"\n":" ");
 out<<"\n";
 out<<"TTY (observed BIOS putchar entries):\n"<<bus.tty<<"\n";return out.str();
}
void install_demo(Bus& bus) {
    // addiu t0,zero,7; addiu t1,zero,5; addu t2,t0,t1; sw t2,256(zero)
    const uint32_t code[]={0x24080007,0x24090005,0x01095021,0xac0a0100};
    for(unsigned i=0;i<4;i++) bus.write32(i*4,code[i]);
}
}
