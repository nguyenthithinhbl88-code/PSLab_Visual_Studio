# PSLab v36 implementation notes

## MCH bootstrap block
16 aligned 32-bit backing slots at 1000F400..1000F4F0, step 10h.
14 newly accepted slots plus the existing RICM/DRD addresses. Unidentified
slots are explicitly experimental storage, not claimed functioning registers.
F410 and RICM reads return zero based on PCSX2's functional reference behavior.
F410 writes are retained for diagnostics. RICM/DRD retain the prior basic
2-device enumeration/CNFGA/CNFGB/DEVID protocol. No RDRAM calibration, timing,
memory resizing or remapping is simulated. Reset clears storage/counters.
Byte/halfword/doubleword/quadword and offsets between slots remain unsupported.

Log(10) SW r0,0(r3), AC600000 at 9FC4135C now advances to 9FC41360.
The last 12 attempted instructions are replayed with entry registers and stack
words reconstructed from logged producers and final snapshot. No full
638699-instruction BIOS replay is claimed.

## VIF memory upload and DMA
BASE/OFFSET VIF1 set base/ofst and TOPS=BASE, clear DBF. ITOP sets ITOPS,
masked to VU0/VU1 address capacity. No VU start/swap is synthesized.
MPG copies NUM (zero=256) 64-bit microinstructions to VU code, modulo region
size. It does not execute microcode. V4-32 UNPACK copies complete raw vectors
to data memory with optional VIF1 TOPS addition, only equal CL/WL and MODE=0.
Other UNPACK formats, masked UNPACK, addition, skip/fill remain rejected.
NUM/progress and waiting state update per payload. Quadwords stage every VU
memory write before commit; a later unsupported command rolls back that packet.
Previous successful packets remain committed, as with a streaming peripheral.

EE DMA0 and DMA1 now accept normal mode, delivering one QW per modeled bus tick.
MADR/QWC progress and completion bits 0/1 update only after VIF accepts the data.
Global enable, disable and D_PCR gates apply. VIF stalls/reverse FDR pause
progress. VIF1 DIR=0, chain/interleave and unmodeled global modes reject.
VIF0 DIR is ignored as documented. GIF source-chain remains intact.
RAM/source bounds stop the appropriate channel and set BUSERR without false
completion. This remains approximate timing; simultaneous channels are serviced
in fixed order rather than a complete hardware arbitration/FIFO scheduler.

## GS pixel pipeline
Untextured PSMCT32/24 rendering supports TEST alpha modes NEVER/ALWAYS/LESS/
LEQUAL/EQUAL/GEQUAL/GREATER/NOTEQUAL and AFAIL KEEP/FB_ONLY/ZB_ONLY/RGB_ONLY.
Z writes must remain masked, so ZB_ONLY failure has no color effect.
DATE/DATM check destination alpha for 32-bit; PSMCT24 ignores DATE.
Z tests NEVER/ALWAYS are handled with writes masked; GEQUAL/GREATER still skip
with a diagnostic. No depth values or Z-buffer execution have been added.

ALPHA selectors implement ((A-B)*C >> 7)+D, with explicit floor for negative
products. COLCLAMP saturates versus wraps RGB. PABE bypasses blending when
source alpha bit 7 is clear. FBA is applied after alpha tests, and FRAME masks
compose with all writes. Reserved ALPHA selectors reject the primitive.
SCANMSK, texture/fog/AA and Z writes/comparison remain unsupported. Functional
integer math is not a claim of bit-exact GS behavior or timing.

## Testing scope
New suites cover MCH slots/protocol and log10 replay; VIF config/MPG/UNPACK,
wrap/NUM=0/rollback; DMA0/1 enable/stall/error/completion and an authored EE
program starting DMA and polling BC0; GS alpha boundary/failure combinations,
blend/clamp/wrap/PABE/DATE/FBA/depth-mask/frame-mask composition.
The old MMIO test now treats only channels above 2 as unsupported; new VIF
suites verify newly supported channels 0/1 with actual memory movement.

Windows/MSVC/GUI runtime could not be tested here. No retail BIOS/game image
is distributed. Full Sony BIOS boot, IOP/VU execution and PS2 games unconfirmed.
See BIOS_PS2_REGISTER_MAP_V36.md for researched hardware map and open gaps.
