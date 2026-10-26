# PSLab v32.3 — BF801472 bootstrap halfword

User log(6): v32.2, instructions=70143, PC=9FC430D4, opcode=A4440000
(SH r4,0(r2)), r2=FFFFFFFFBF801472, r4=1. GS SMODE/timing writes have
already executed. No GS primitives or GIF payloads have been submitted.

The new mapping stores a 16-bit value at physical 1F801472 and shares it
across KSEG0/KSEG1 aliases. Reset clears it. Reads/writes have diagnostic
counters. Only aligned 16-bit accesses are supported. Byte/word/quadword,
unaligned, partial and neighboring accesses remain errors without mutation.

Reference: PCSX2 1.6.0 IopHwWrite.cpp uses generic backing storage for
unhandled page-1 halfword writes, including this address. No peripheral
identity, bit meaning, IRQ, timing, or handshake is inferred. This is a
narrow compatibility latch, not full IOP hardware emulation. Original code;
no external emulator implementation was copied.

Regression covers aliases/reset/truncation/atomic rejection and the supplied
64-attempt tail, including GS timing register writes and branch delay slots.
It reaches 9FC430D8. Tail entry r7 nonzero is inferred from the logged untaken
BEQ; r8=0 comes from the final snapshot. Initial delay=true comes from trace.
This does not replay the full 70143-instruction boot. No Sony BIOS is included.
Full BIOS boot, screen output and PS2 games remain unconfirmed.

Build in Visual Studio 2022: open project using supplied instructions, choose
Release x64, rebuild pslab, verify title v32.3. Load BIOS (test), then Run
continuously. Export a new PS2 log if execution stops again.
