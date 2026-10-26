# PSLab v33 — grouped IOP bootstrap registers and memory

## Reported failure
User log(7), v32.3: 70145 completed instructions, PC 9FC430DC,
A4600000 = SH r0,0(r3), r3=FFFFFFFFBF801470. The previous SH at
9FC430D4 wrote 1 to BF801472 successfully. GS timing registers were initialized,
but no rendered primitives or GIF payloads are present yet.

The old one-address latch approach stopped at the next neighboring address.
This update implements groups with distinct access rules instead of accepting
all unknown MMIO as storage.

## Register coverage
71 newly supported register locations (plus the existing 1472 latch):

| Group | Locations | Model |
|---|---:|---|
| SSBUS/base/delay/RAM-size/ICFG | 21 words | Configuration storage; byte/halfword/word lane merge |
| IOP interrupt controller | 3 words | I_STAT write-zero-clear, I_MASK storage, I_CTRL bit 0/read-clear |
| 13 IOP DMA channels | 39 words | MADR low 24 bits, BCR, stopped CHCR configuration |
| SIF0 DMA tag address | 1 word | TADR low 24 bits |
| DMA globals | 6 words | DPCR/DPCR2, DICR/DICR2 W1C flags, DMACEN/DMACINTEN masks |
| Bootstrap BF801470 | 1 halfword | Separate storage from BF801472 |

Configuration word whitelist (physical):
1F801000..1F801020 step 4; 1F801060;
1F801400..1F801420 step 4; 1F801450 and 1F801454.
These are storage models: address registers do not actually remap devices,
change RAM size, apply bus waitstates, or start IOP execution. Configuration
reset policy is zero, not a claimed hardware power-on dump.

DMA channel MADR/BCR/CHCR accept aligned halfword and word accesses;
only channel 9 exposes TADR. A CHCR write setting start bit 24 fails before
changing the register or write counters. No DMA completion is fabricated.
DPCR reset is 07777777; other new globals use zero as initial model state.
DICR/DICR2 flags clear on writing 1; channel flags are never raised by a
nonexistent transfer. Combined master summary honors global interrupt controls.
IRQ3 can be raised in IOP I_STAT, but no IOP CPU exception is executed.

1470 and 1472 remain independent aligned halfword-only bootstrap storage.
Their exact hardware bit meanings and side effects are still unverified.
Unknown neighbors, unsupported widths and unaligned accesses still fail.

## Memory preparation
EE physical 1C000000..1C1FFFFF maps a distinct 2 MiB IOP RAM buffer.
VU0 code 11000000..11000FFF and data 11004000..11004FFF;
VU1 code 11008000..1100BFFF and data 1100C000..1100FFFF.
This is 40 KiB of VU code/data storage, without VU execution or VIF uploads.
Physical/KSEG0/KSEG1 aliases share data. CPU byte through quadword accesses
use the normal memory paths; stores validate destinations before mutation.
Like EE RAM, the new memory survives an EE CPU reset; fresh instances zero it.

## Regression and scope
New tests exercise every configuration word and all 13 channel banks, partial
lanes, aliases, masks, DMA start rejection, IRQ W0C/read-clear/W1C, snapshots
without side effects, all memory ranges and boundaries. An authored EE program
uses SQ/LQ to move 128-bit data through IOP RAM into VU1 data memory.

The 64-attempt tail from log(7) now reaches 9FC430E0 after both SH writes.
Tail entry delay=true is logged; r8=0 comes from the final register snapshot.
The entry register snapshot was not supplied. This is not a full replay of
70145 instructions and not proof of complete Sony BIOS startup.

IOP R3000A execution, IOP timer devices, SIF transfer queues/RPC, VIF/VU
execution, SPU2, CDVD and complete PS2 game support are still absent.
Adding registers alone cannot replace those engines.

## Run on Windows
Extract into a fresh folder, open using the existing Visual Studio 2022
instructions, build Release x64, launch pslab.exe and verify title v33.
Open the PS2 lab, Load BIOS (test), then Run continuously. If it stops, export
the new PS2 log. Rebuild is required: the ZIP contains source, not a prebuilt
Windows executable. MSVC/Windows runtime verification was unavailable here.

## Primary references (original implementation; no external code copied)
PS2SDK SSBUS controller source: base/delay tables and common delay word:
https://raw.githubusercontent.com/ps2dev/ps2sdk/master/iop/system/ssbusc/src/ssbusc.c
Retrieved directly from the upstream repository during this update.
PS2SDK register declarations including BF801470/BF801472/ICFG/RAM-size:
https://israpps.github.io/ps2sdk/iop__regs_8h_source.html
ps2tek: IOP interrupt/DMA register semantics and EE IOP/VU memory map:
https://psi-rockin.github.io/ps2tek/index.html
PCSX2 IOP page-1 default backing storage (reference for bootstrap latches):
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/ps2/Iop/IopHwWrite.cpp
