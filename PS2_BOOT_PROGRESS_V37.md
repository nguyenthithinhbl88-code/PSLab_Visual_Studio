# PSLab v37 — MCH lane fix and faster BIOS execution

## Actual new failure
The supplied v36 log completed 15566515 instructions, then stopped at
9FC42140, opcode 9463F480 = LHU r3,-0xB80(r3). The source base is
FFFFFFFFB0010000, effective address B000F480. MCH[1000F480]=7C30 already
exists, but v36 only accepted word accesses. The fix supplies the low 16 bits
and advances PC to 9FC42144. It does not skip the instruction or change BIOS.

The earlier screenshot was still running a Count-polling loop at 9FC42930;
it was not showing a CPU fault. The later log proves execution passed that
loop. Its MCH initialization traffic has progressed, but the GS output circuit
remains disabled. No actual BIOS menu rendering has been demonstrated.

## MCH access-width correction
All 16 previously modeled MCH slots accept aligned 1/2/4-byte accesses to
their low word. Byte/halfword reads select the little-endian lane of a single
word-handler result, matching the PCSX2 reference's narrow read routing.
RICM/DRD side effects happen once per access. Partial writes merge with the
stored word, then invoke the word handler once; that merge policy preserves
untouched backing bits and is not a hardware-verified calibration behavior.

Offsets 4..15 between MCH word slots remain unsupported; unaligned accesses,
8-byte and quadword accesses remain unsupported. Unknown register meanings
from v36 remain explicitly experimental backing storage.

## CPU throughput changes
The last 64 instruction attempts now use a fixed-size binary trace ring.
String formatting occurs only when exporting/updating diagnostics. Instruction
fetch is performed once; trace recording no longer performs a second read,
which also removes duplicate MMIO side effects when fetching unusual addresses.
The public execute() entry still fetches/delegates normally when called directly.

RAM, BIOS, scratchpad, IOP and VU memory access takes an aligned direct-byte
path before expensive MMIO dispatch. It preserves little-endian values,
region bounds and alignment guards. No guest instruction or hardware tick is
omitted and Count is not artificially accelerated.

The Windows run scheduler now permits up to 250000 instructions per batch,
checking its time budget every 64 instructions. Fast run defaults ON with a
12 ms soft budget per 16 ms timer event; OFF uses the previous 4 ms budget.
Pause/Stop still retain CPU state. The button toggles the setting and pauses;
press Run continuously to resume. Host MIPS is labeled separately from FPS.
MSVC/Windows runtime validation was unavailable here.

## Boot/display evidence in logs
New diagnostic summary reports PMODE, GIF QWs, primitives and skipped draws.
It reports disabled display explicitly. It recognizes the exact seven-opcode
Count polling loop observed in the screenshot only after a matching complete
trace sequence; it prints Count/start/threshold without modifying CPU state.
A matching loop is an observation, not proof of an infinite hang.

This update does not inject a PS2 menu, set PMODE behind the BIOS, fake an
IOP-ready reply, or claim BIOS compatibility based on project-authored demos.
To display the actual BIOS interface, the BIOS must reach its drawing code
and the required IOP/SIF/VU/GS functionality must exist. Those engines remain
incomplete. The delivered result fixes this concrete stop and reduces the
cost of reaching the next genuine execution result.

## Checks and benchmark
Tests cover all ordinary MCH narrow lanes/partial writes/aliases, guards,
reported LHU, single-fetch MMIO counts, chronological ring retention and fetch
fault entries, a Count loop exiting at exactly 42 instructions/ticks, and
fast-path memory region edges. Legacy halfword-rejection checks were updated
because halfword MCH access is now intentional.

PS2_BOOT_BENCHMARK_V37.txt records three runs each of v36/v37 on a project-
authored one-million-instruction Count loop, same GCC -O2 and host. Each run
has identical instruction/Count/PC/GPR2/video signatures. These host timings
are synthetic, subject to contention, and not Sony BIOS FPS or Windows results.

Extract to a fresh directory, build Release x64, verify title v37. Load BIOS
(test), leave Fast run ON, Run continuously. If another stop occurs, Save log.
No Sony BIOS/game files are distributed; full BIOS menu boot is unconfirmed.

## Sources consulted; original implementation
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwRead.cpp
Narrow reads extract a byte/halfword from one aligned word read.
https://psi-rockin.github.io/ps2tek/index.html
COP0 Count increments with emulated EE cycles; GS display/VRAM configuration.
https://pcsx2.net/docs/advanced/psx/
Relationship between EE, IOP and the hardware communication blocks.
No third-party emulator code copied into this update.
