#include "ps2_ee.hpp"
#include <sstream>
#include <stdexcept>
namespace ps2 {
namespace {
const char* cache_name(unsigned operation){
 switch(operation){case 7:return "IXIN";case 0xc:return "BFH";case 0x14:return "DXWBIN";case 0x16:return "DXIN";case 0x18:return "DHWBIN";case 0x1a:return "DHIN";case 0x1c:return "DHWOIN";default:return nullptr;}
}
}
void Ee::cache_maintenance(unsigned operation,uint32_t address){
 if(!(cop0[12]&6)&&(cop0[12]&0x18))throw std::runtime_error("EE CACHE requires kernel mode; privilege exception entry unimplemented");
 if(!cache_name(operation))throw std::runtime_error("EE CACHE operation unsupported: tag/data array access and other cache operations are not modeled");
 // The interpreter has no I-cache/BTAC, dirty D-cache, or decoded-code cache.
 // Stores already reach shared memory synchronously. Maintenance therefore has no
 // outstanding line data to flush/invalidate in this explicitly coherent model.
 // The operand is an address, not a memory load/store; never touch MMIO here.
 cache_operations[operation]++;cache_last_address=address;cache_last_operation=operation;
}
std::string Ee::cache_diagnostic()const{
 std::ostringstream o;o<<"CACHE maintenance: coherent-memory functional model only; no real I/D-cache, tags, dirty lines, BTAC, timing or cache-induced TLB faults.\r\n";
 for(unsigned n=0;n<32;n++)if(auto name=cache_name(n))o<<"CACHE "<<name<<"="<<std::dec<<cache_operations[n]<<" ";
 o<<"last_op="<<std::hex<<cache_last_operation<<" last_address="<<cache_last_address<<"\r\n";return o.str();
}
}
