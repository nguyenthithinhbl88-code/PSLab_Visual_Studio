# v20.1 - DMA register byte/halfword reads

The supplied log ps1_bios_log(20261026-052215).txt contains the BIOS loader's Execute message for SLUS_001.06;1, PC0=800F2984h and a 120000h-byte executable at 80010000h. Game code is executing in RAM. It stops at 80127F40h on opcode 90640002h (LBU a0,2(v1)), with v1=1F8010F4h and DICR=008A0000h. The requested byte is therefore 8Ah at 1F8010F6h. The controller already exposes the full word, but the byte read path did not decode DMA registers.

The byte read path now uses the existing DMA register decoder and selects the requested little-endian lane. Halfword reads compose two valid lanes. This covers MADR/BCR/CHCR for DMA0..6, plus DPCR and DICR, including KSEG0/KSEG1 aliases. Reads do not acknowledge IRQ flags or start transfers. Unsupported register offsets and unaligned halfwords still report errors.

The new regression executes the exact logged opcode/address at the logged PC and reproduces the old failure before the fix. It then checks every implemented DMA register's byte and halfword lanes, aliases, signed/unsigned MIPS loads, load delay and absence of read side effects. All 27 normal test suites pass with C++17, -O2, -Wall, -Wextra, -Werror. The new regression also passes AddressSanitizer and UndefinedBehaviorSanitizer.

The log also shows 595 CD sectors read and successful MDEC table uploads. It contains no decoded MDEC macroblocks and does not establish that gameplay or FMV rendering works. Real gameplay remains unverified; the code is validated here with original fixtures, not the user's commercial disc or BIOS.

Primary hardware reference consulted for I/O access widths: https://psx-spx.consoledev.net/unpredictablethings/ . This patch changes supported read widths, not DMA write or completion semantics.
