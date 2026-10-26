# PSLab v34 — EE serial and VIF bootstrap groups

## New log and root cause
User log(8) from v33: 633468 instructions completed; PC=9FC43374;
opcode AD430000 = SW r3,0(r10), r10=FFFFFFFFB000F100, r3=20.
This is SIO_LCR (EE serial line control), not an IOP register. The serial
initialization routine computed its baud-rate argument before reaching the
unmapped register. v33 already passed both IOP bootstrap halfword writes.
No GS drawing/game startup is established by this log.

## Serial group
Eight named locations now have handlers (TXFIFO existed previously as only a
byte console hook; the other seven locations are new):

| Physical | Register | Functional model |
|---|---|---|
| 1000F100 | SIO_LCR | configuration storage |
| 1000F110 | SIO_LSR | zero: no RX data or simulated line errors; writes ignored |
| 1000F120 | SIO_IER | configuration storage; serial IRQ scheduling absent |
| 1000F130 | SIO_ISR | zero: TX sink ready, empty RX, no pending IRQ |
| 1000F140 | SIO_FCR | control storage; count reset requests; no buffered traffic |
| 1000F150 | SIO_BGR | baud configuration storage; no baud clock simulation |
| 1000F180 | SIO_TXFIFO | write-only console sink |
| 1000F1C0 | SIO_RXFIFO | byte-only empty read returns zero; writes unsupported |

Control/status registers accept aligned byte/halfword/word lanes within their
32-bit slot; aliases share state. Configuration bits are retained as storage,
not a hardware-verified reserved-bit mask. Status values are functional policy
for a disconnected serial port, based on SDK polling and emulator references.
There is no host serial connection, RX injection, loopback or simulated input.

TX captures printable text/newline/CR/tab, filters other bytes to '.', retains
last 16 KiB, and counts bytes. Halfword/word writes send little-endian bytes,
a compatibility choice also seen in PCSX2 and not established on real hardware.
FIFO reset requests do not erase already captured console history. Exported
snapshots do not perform MMIO reads. Reset clears state, counters and capture.

## VIF0 and VIF1 group
42 valid register slots: VIF0=19, VIF1=23. Word-only MMIO, physical/kernel aliases.
STAT, FBRST, ERR, MARK, CYCLE, MODE, NUM, MASK, CODE, ITOPS, ITOP,
R0-R3, C0-C3; VIF1 additionally BASE, OFST, TOPS, TOP.
VIF0 addresses at offsets A0/B0/C0/E0 and both banks at F0 remain unsupported.

FBRST resets the bank and partial payload; force break/STOP set stall flags,
and stall cancel clears VSS/VFS/VIS/INT/ER0/ER1. With no executing command,
STOP is immediate. FBRST reads return zero as explicit model policy.
ERR stores bits 0..2. Writing MARK retains low 16 bits and clears MRK.
STAT permits FDR writes only on VIF1. Other command-produced registers ignore
CPU writes. Reset policy clears modeled VIF state; not a hardware reset dump.

CPU SQ at 10004000 (VIF0) or 10005000 (VIF1) decodes:
NOP, STCYCL, STMOD, MARK, STMASK, STROW, STCOL.
Mask/row/column payloads can span several quadwords. STAT reports waiting for
payload versus idle, CODE records command and MARK sets MRK. A quadword is
validated on a copy before commit, so an unsupported fourth command cannot
partially update the bank. Stopped/reversed FIFO writes reject atomically.

Interrupt-bit commands require ERR.MII=1 (suppressed); actual interrupt-bit
scheduling is explicitly unsupported. UNPACK, MPG, VU start, PATH2 DIRECT,
flush dependencies, BASE/OFFSET/ITOP commands and VIF DMA are still unsupported.
Reading a register does not mean its underlying execution engine is present.
No instruction is patched out of the BIOS and no fake IOP-ready flag is set.

## Regression coverage and limits
Serial suite: every named location, lanes/aliases, SDK-style initialization,
TX ordering/filtering/bounds, empty RX, guards, reset, side-effect-free logs.
The reported SW advances to 9FC43378. The final 20 attempted instructions of
log(8), starting 9FC43320, now replay to the same PC. Entry r4=47FC4 is taken
from final r16 and the preceding logged JAL delay slot; r5=9600 from logged
ORI; r6..r9 use zero consistent with the final snapshot/branch. The missing
entry snapshot and initial polling loop are not reconstructed. This is not a
633468-instruction replay or proof of complete Sony BIOS boot.

VIF suite: all 42 slots, access rules, read-only behavior, masks/reset/stalls,
all seven commands, payload splits, whole-packet rollback, interrupt guards,
and an authored EE SQ program reaching the VIF1 decoder through its alias.
No Sony BIOS/game images are bundled. IOP/VU/SPU2/CDVD execution is still absent.

Build Release x64 in Visual Studio 2022, launch pslab.exe, verify v34,
Load BIOS (test), Run continuously. Export a fresh log if execution stops.
The package contains source; Windows/MSVC build/runtime was unavailable here.

## Primary sources consulted; implementation written originally
PS2SDK serial register addresses, FIFO resets, initialization and polling:
https://ps2dev.github.io/ps2sdk/sio_8h.html
https://ps2dev.github.io/ps2sdk/sio_8c_source.html
PCSX2 serial status and multi-byte TX policy:
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwRead.cpp
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwWrite.cpp
Current upstream versions were also retrieved directly over HTTPS for reference.
https://raw.githubusercontent.com/PCSX2/pcsx2/master/pcsx2/HwRead.cpp
https://raw.githubusercontent.com/PCSX2/pcsx2/master/pcsx2/HwWrite.cpp
ps2tek VIF register permissions, reset/stall controls and configuration commands:
https://psi-rockin.github.io/ps2tek/index.html
No external emulator code copied into the project.
