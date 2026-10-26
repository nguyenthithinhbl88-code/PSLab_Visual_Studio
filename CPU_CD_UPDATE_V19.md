# PS1 Lab v19.0 - CPU and CD transfer corrections

## What the real logs prove

Logs (9) and (10) stop at the same instruction count (197837717), same CPU state, and same invalid load at 80042080 using s7=c0ae7024. Both contain 17 CD commands, 8 sector reads and a successful emulated SCEA GetID. They do not contain enough history to identify the first incorrect value. A successful real GTA boot is still unverified.

## Functional corrections

- CPU: consecutive GPR loads to the same destination now suppress the older writeback. The new load still reads its address from the pre-load register state. Ordinary loads and MFC0/MFC2 use the same rule; LWL/LWR retain merge forwarding. A faulting second load leaves the earlier completed result intact. IRQ entry still commits a pending load before the handler.
- CPU/DMA: SB/SH at a DMA register's aligned word address pass the full GPR bus value, retaining high DMA control/IRQ bits. RAM writes stay narrow. Other subword MMIO behavior, including accesses to other byte lanes, is unchanged.
- CD: drive sector storage and host data FIFO are now separate. Clearing BFRD flushes the host FIFO; requesting data reloads the buffered sector without incrementing LBA. Before this change, the same sequence erased the only copy of the sector.
- CD: sectors are spaced at 451584 approximate CPU ticks at 1x, or 225792 at 2x. Previously sector delivery followed ACK/FIFO consumption nearly immediately. First delivery also waits a sector interval after command ACK transport. Pause cancels further arrivals while retaining the current buffered data; Stop/Init/reset clear it.
- CD hardware reads use their sector data window independently of ISO parsing, including a bounded 2048-byte window for Mode2 Form2. The ISO filesystem reader continues to reject Form2 as filesystem data. This does not implement XA audio.

The BFRD defect provides a plausible mechanism for shifted or missing boot data. It has been reproduced with original fixtures; it has not been proven to be the cause of the user's specific invalid pointer.

## Diagnostics for the next real run

Save Log now includes 128 CPU instructions with source operand values and pending load state, sector LBA and payload prefix, BFRD requests/FIFO loads, 32 CD DMA transfer records (RAM address, LBA, word count, first words and FNV1a checksum), and bounded RAM windows at 10000h and the current a0 pointer. No invalid memory access is replaced with a zero read; strict unsupported bus errors remain visible.

## Validation and limits

New CPU and CD regressions fail against the v18.2 core and pass after the corrections. The normal build covers 24 suites with GCC C++17, -O2, -Wall, -Wextra, -Werror. It includes original MIPS CPU/CD programs and byte-for-byte partial and multi-sector DMA checks. The two new regression suites also pass AddressSanitizer and UndefinedBehaviorSanitizer. The updated Form2 hardware-window and DMA history assertions pass in the disc/CD transfer suites.

One interpreter instruction still approximates one CPU clock. The sector model applies conservative backpressure when IRQ/data are unconsumed; it does not yet emulate real drive overrun/skipping or the full multi-slot sector SRAM. Seek/mechanical delays, second-response timing, cache/waitstates, CD-DA/XA and MDEC remain incomplete. The Win32 UI cannot be built or run in this Linux environment. No Sony BIOS or commercial disc is included.

## References consulted

- PSX-SPX CPU Specifications, Load/Store and subword MMIO cautions: https://psx-spx.consoledev.net/cpuspecifications/
- PSX-SPX CDROM Drive, host interface and INT1 scheduling: https://psx-spx.consoledev.net/cdromdrive/
- DuckStation CPU state declaration (separate current/next load-delay state): https://github.com/stenzek/duckstation/blob/master/src/core/cpu_core.h

Implementation is original project code. These references describe hardware behavior or state organization, not evidence that GTA has run here.
