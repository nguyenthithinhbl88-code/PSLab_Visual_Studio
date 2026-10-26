# v19.1 - CD Init completion IRQ spacing

## Evidence

The user's ps1_bios_log(20261026-045735).txt has 1970 commands but only 8 sectors. The retained CD history repeatedly sends Init (0Ah). In each cycle, INT2 is delivered exactly one interpreter tick after ACK of INT3 (for example 1086322782 -> 1086322783). The BIOS polls a zero state at RAM 000091c4 and eventually retries Init. CPU reserved exceptions and BIOS_SystemError remain zero. The boot logo DMA records now contain the TMD header at RAM 10000h; the former invalid-pointer failure is absent in this run.

The host's I_STAT is edge latched. If INT2 reasserts the CD source during the INT3 handler and that handler subsequently clears I_STAT, the second event can be lost. This ordering has been reproduced with a synthetic host sequence and an original MIPS ISR. It is a strong explanation for the observed Init loop, not verification of a real GTA boot.

## Correction

After a CD interrupt flag transitions from nonzero to zero through ACK, keep the CD source low for at least 1000 approximate CPU ticks before another queued response or sector IRQ can assert it. This is a conservative spacing policy informed by DuckStation's MINIMUM_INTERRUPT_DELAY=1000 reference constant. First-command transport latency and sector cadence continue advancing independently; the queued result is preserved. A partial ACK that leaves flags set does not manufacture a source-low transition. Reset clears the spacing state.

This change preserves edge-triggered I_STAT behavior. It does not force its bit high while the CD source remains asserted. Full Init/mechanical completion times are still approximate; this is not a complete HC05 scheduler.

Save Log now includes irq_ack_delay and host-ack history entries when the CPU clears CD IRQ2 in I_STAT. This allows the next real run to show the source ACK, host ACK and subsequent completion in order.

## Validation

The new regression fails against v19.0. After the change it checks the entire 1000-tick boundary, delayed host I_STAT acknowledgment, and actual MIPS code receiving exactly two IRQ entries and a completion marker after one Init command. The full 25-suite native build uses GCC C++17 with -O2 -Wall -Wextra -Werror. All 25 suites passed. The Init IRQ regression also passed AddressSanitizer/UndefinedBehaviorSanitizer. After adding host-ACK logging, the spacing/protocol/BIOS I/O suites were rebuilt and passed again.

No real Sony BIOS/game or Windows SDK is available in this environment. No change is made to boot routing, commercial executable loading, GTE, audio or input.

## Primary references

- PSX-SPX Interrupts: https://psx-spx.consoledev.net/interrupts/ (edge-latched I_STAT and ACK ordering).
- PSX-SPX CD-ROM Drive: https://psx-spx.consoledev.net/cdromdrive/ (first and second command responses).
- DuckStation cdrom.cpp: https://github.com/stenzek/duckstation/blob/master/src/core/cdrom.cpp (minimum interrupt spacing reference).
