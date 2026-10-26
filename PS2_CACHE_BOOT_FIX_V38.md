# PSLab v38 — BIOS cache maintenance bootstrap

Log v37 (20261026-135213) completed 15803558 instructions and printed
“Total accessable memory size: 32 MB”. It stopped at BFC00904 with
BF470000 = CACHE IXIN, using r26=0 as the address. The GS still reports
PMODE=0 and zero GIF QWs/primitives; this is not a graphics-output failure.

The patch accepts seven explicitly selected maintenance operations:
07 IXIN, 0C BFH, 14 DXWBIN, 16 DXIN, 18 DHWBIN, 1A DHIN, 1C DHWOIN.
These operate in the project's coherent-memory functional model: there is
no separate instruction cache, branch-target cache or dirty data cache.
Stores already update the shared memory used by CPU fetch and DMA. Therefore
there are no outstanding cache lines to flush/invalidate for these operations.
The instructions advance normally and are counted with effective addresses.
This is not implementation of physical PS2 cache lines, timing or coherency bugs.

PCSX2's Cache.cpp identifies opcode 07 as IXIN and treats it as having no
additional effect when no instruction cache exists. That reference supports
the reported instruction's handling. For the selected D-cache maintenance
operations, PSLab uses its own coherent-memory policy rather than copying
PCSX2's actual D-cache line/writeback implementation.

Cache tag/data array operations and all unlisted CACHE encodings still stop
explicitly. Kernel privilege is checked; EXL/ERL override KSU. Unsupported
privilege exceptions stop explicitly rather than entering an architectural
exception vector. Address operands do not cause ordinary bus/MMIO reads or
writes. Cache-related TLB faults and permission checks remain unmodeled.
Counters, last operation and effective address are exported with this scope.

The reported instruction now advances to BFC00908. Regression also replays
the final seven attempted instructions supplied in the log: stack setup,
JAL/delay slot, r26 zeroing, SYNC.P and IXIN. That replay is not the complete
15.8-million-instruction Sony BIOS execution. An authored program stores a
new instruction into RAM, uses SYNC/IXIN, jumps to it and verifies execution.
Tests cover all selected operations, signed offsets, unchanged GPR/memory/tags,
MMIO safety, privilege/unknown-operation guards, branch delay slots and reset.

Extract to a fresh folder; build Release x64; verify PSLab v38; Load BIOS
(test), Fast run ON, Run continuously. If another stop occurs, export its log.
No Sony BIOS is bundled. Windows/MSVC/GUI runtime could not be tested here.
The actual PS2 BIOS menu and PS2 games remain unconfirmed; IOP/SIF/VU and
other hardware execution still require work.

Primary reference consulted (original project implementation, no code copied):
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/Cache.cpp
