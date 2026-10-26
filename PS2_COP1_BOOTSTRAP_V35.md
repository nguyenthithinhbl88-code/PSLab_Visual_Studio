# PSLab v35 — COP1 register and memory transfers

## Reported stop
User log(9), v34: 633522 completed instructions; PC=9FC434A4;
E7AC00B8 = SWC1 f12,0xB8(sp). SP=70003E90, destination=70003F48
in the existing scratchpad. Serial initialization completed (LCR=20, BGR=1D).
This is a missing CPU/FPU instruction, not an unmapped peripheral register.
The trace is a function prologue saving integer arguments and a floating-point
register before processing a format string. No complete BIOS boot is established.

## Implemented
- 32 writable 32-bit FPRs (f0 is writable). Raw bits are preserved, including
  sign bits and unusual exponent patterns; there is no host float conversion.
- FCR0 read-only identity 00002E00 (implementation 2E, revision 0 model choice).
- FCR31 writable C/cause/sticky fields mask 0083C078, fixed bits 24 and 0 set.
- LWC1/SWC1 use the existing aligned 32-bit shared memory/MMIO paths.
- MTC1 truncates GPR low 64 to 32 bits; MFC1 sign-extends to GPR low 64 while
  preserving upper 64. Writes to GPR zero are discarded.
- CFC1/CTC1 for control registers 0 and 31. Other reserved control indices stop
  explicitly, rather than implementing undocumented aliases.
- MOV.S copies raw FPR bits. Arithmetic/conversion/compare/ACC remain absent.
- BC1F/T/FL/TL use FCR31.C with normal delay-slot and likely-annul behavior.
- Status.CU1 is checked before register or memory side effects. With CU1=0,
  execution stops explicitly; architectural coprocessor-unusable exception
  delivery is not implemented by this patch.
- Reset clears FPRs deterministically and initializes the modeled FCR state.
  FPR zeros are a model policy, not a verified hardware power-on snapshot.
- Logs include all FPR bits and FCR0/FCR31 with the supported-scope label.

Unknown encodings, unsupported arithmetic, unaligned/unmapped loads/stores
still stop. Instructions are not skipped and BIOS code is not modified.
No FPU arithmetic compatibility claim follows from adding the register bank.

## Validation
All 32 FPRs are tested through GPR moves, MOV.S and signed-offset LWC1/SWC1.
Patterns include signed zero, normal values and non-IEEE/raw patterns.
Tests cover GPR high-half/zero behavior, FCR masks/read-only identity, all
8 BC1 condition/type combinations, delay-slot stores, CU1, invalid encodings,
alignment, bus errors, scratchpad/IOP/VU memory and a serial MMIO store.

The reported SWC1 reaches 9FC434A8 with a nonzero test pattern, proving a
real 32-bit store. The final 18 attempted instructions (9FC43460..9FC434A4)
are replayed against reconstructed entry arguments: old SP=70003F90, final
argument registers and RA from the supplied log; format byte 23 inferred
from final r2 and the only LB in that prologue. f12=7FC12345 is an authored
test pattern: the user log did not contain FPR contents. This is not a full
633522-instruction replay or proof of correct Sony BIOS startup.

Build Release x64 in Visual Studio 2022, launch pslab.exe, verify v35,
Load BIOS (test), Run continuously. Export a new log if execution stops.
No Sony BIOS or game images are included. Windows/MSVC build/runtime could
not be tested in this environment. IOP/VU execution, SPU2/CDVD and full BIOS
boot/game compatibility remain unfinished.

## Primary references (original implementation)
EE Core User's Manual, FPU registers/control (sections 8.2–8.3), FPU features:
https://docs.alexrp.com/mips/ee.pdf
PCSX2 FPU register/memory transfer reference behavior:
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/FPU.cpp
ps2tek EE COP1 and load/store decoding tables:
https://psi-rockin.github.io/ps2tek/index.html
No external emulator code was copied into the project.
