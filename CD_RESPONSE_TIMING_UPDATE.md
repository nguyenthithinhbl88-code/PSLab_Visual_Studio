# v18.2 - CD response timing

## Evidence from ps1_bios_log(8).txt

The mounted GTA disc has 11 tracks and SYSTEM.CNF resolves SLUS_001.06. The BIOS sends Test(19h,20h), receives 95 07 24 c1, ACKs it, then sends Getstat(01h), receives status 02h and ACKs it. No GetID, Setloc or ReadN occurs; sectors_read remains zero. There are no reserved instructions or BIOS SystemError. The previous v18.1 protocol correction did not resolve retail boot. Many VSync timeouts remain.

This log does not contain the BIOS instructions that choose the menu path. It therefore does not establish a unique root cause.

## Change

The old controller made a response visible at the first CPU instruction after the command write. The new first-response countdown uses 0xc4e1 approximate CPU clocks for ordinary commands and 0x13cce for Init and ReadTOC. These values are measured averages documented by PSX-SPX, not exact per-command or per-model firmware timings. BUSYSTS stays set until delivery. Reset cancels the scheduled response. Interrupt/ACK queues and unread-result draining are preserved.

History now records clock timestamps and individual RESULT reads so the next log distinguishes values queued by the controller from bytes consumed by the BIOS. Polling reads are not logged, keeping history useful.

This is a correction to a verified timing defect, not a confirmed explanation for the menu route. It does not bypass the BIOS or fabricate a successful game load. Mechanical latency, later responses, sector delivery, and CPU instruction costs still need more accurate scheduling.

## Validation

22 native C++ test suites pass with GCC C++17, -O2, -Wall, -Wextra, -Werror. The CD protocol test checks no early result/IRQ, exact first-response boundaries, initialization latency, reset cancellation, queued commands, and a real MIPS polling program. The original homebrew disc still runs through CD registers and DMA3. No real Sony BIOS, GTA image, Windows SDK or Visual Studio runtime is available here; commercial boot and the Win32 UI are unverified.

Reference: https://psx-spx.consoledev.net/cdromdrive/#cdrom-response-timings
