# v20.0 - MDEC and DMA video path

The v19.1 screenshot stops on an unsupported write to 1F801824h, with r2=80000000h, r3=1F801824h, and the next PC at 8012D26Ch. The address/value match a device reset for the missing MDEC peripheral. The screenshot alone does not establish that GTA gameplay or a specific movie has started.

## Implemented

- MDEC word registers at 1F801820h and 1F801824h, including uncached aliases, reset/abort, DMA enable gates, status and parameter counts.
- Quantization and custom transform table uploads, incremental run-length coefficient parsing, a fixed-point two-pass inverse transform, and pixel conversion/packing for monochrome and RGB output.
- DMA0 transfers from RAM into the decoder. DMA1 waits for decoded output and places the four color blocks into a row-major macroblock in RAM. Both support ordinary forward manual/request modes, bounded queues, resuming stalled transfers and completion IRQ bookkeeping.
- Save Log records MDEC state and DMA0/1 progress. The UI adds a decoded-block counter and status. CPU subword stores at the MDEC word register addresses retain the full GPR bus value, like the existing DMA register path.

## Verification

26 native C++ test suites pass with GCC C++17, -O2, -Wall, -Wextra and -Werror. The new MDEC suite also passes AddressSanitizer and UndefinedBehaviorSanitizer. It checks an original MIPS reset sequence at the reported address, table uploads/counts, neutral/DC pixel goldens, a nonzero AC/zigzag/custom-scale golden, chroma conversion, split RLE blocks, signed output, bit15, reset cancellation, DMA gates, byte-for-byte color output, four-block row reordering and two-macroblock DMA under backpressure. Existing CPU, BIOS, CD and original homebrew tests still pass.

## Limits

This is a functional decoder, not hardware-exact video timing. There is no MDEC execution-cycle scheduler, exact intermediate rounding or exact early deassertion of output DREQ. CPU writes to a full input FIFO report an explicit error rather than stall the processor. Backward/chopped MDEC DMA and mixed CPU/DMA reads within one color macroblock are not validated. Word accesses are the supported MMIO path. No real commercial movie/disc/BIOS or Windows UI can be run in this environment, so GTA and real FMV compatibility remain unverified.

No BIOS bytes, game video or game executable are included. All test streams and MIPS instructions are original fixtures.

Reference used for the hardware interface and decoding model: https://psx-spx.consoledev.net/macroblockdecodermdec/ . The implementation is original project code; numerical results should be compared against real hardware before claiming bit accuracy.
