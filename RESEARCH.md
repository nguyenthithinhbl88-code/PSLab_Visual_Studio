# Nguồn đối chiếu cho bản 17

Đã tra tài liệu trên mạng trước khi bổ sung mã. Code mới được viết trong dự án;
không chép phần triển khai SPU của DuckStation. Trang spu.cpp của DuckStation
không truy cập được qua công cụ duyệt trong phiên này.

- [PSX-SPX: SPU](https://psx-spx.consoledev.net/soundprocessingunitspu/): bản đồ thanh ghi,
  các hệ số số học ADPCM/Gaussian, ADSR, KON/KOFF, ENDX, volume, capture/IRQ9,
  mạng IIR/COMB/APF và cách wrap bộ nhớ reverb.
- [PSX-SPX: CD format](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromformat/): hệ số predictor ADPCM.
- [PSX-SPX: timers](https://psx-spx.consoledev.net/timers/): nguồn clock và chế độ đồng bộ.
- [PSX-SPX: GTE](https://psx-spx.consoledev.net/geometrytransformationenginegte/): những lệnh COP2
  còn chưa được triển khai. Việc thêm âm thanh không hoàn thiện phần này.
- [Microsoft: About timers](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-timers):
  WM_TIMER là thông báo giao diện với thời điểm gần đúng.
- [Microsoft: waveOutWrite](https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutwrite):
  vòng đời buffer âm thanh; prepare trước khi gửi, chỉ dùng lại buffer đã trả về.

Hệ số phần cứng trong spu_gauss.hpp là dữ liệu số được ghi trong PSX-SPX.
Không có BIOS, bản ghi tiếng startup Sony hay game thương mại trong gói.
Các tài liệu được đối chiếu ngày 25/10/2026.

## Bổ sung bản18

Đối chiếu GTE register file, các lệnh/tính cờ, UNR division, COP2 load delay
và BIOS A0(40) SystemError tại các trang GTE/CPU/BIOS nêu trong GTE_BIOS_UPDATE.md.

## Bổ sung18.1

Đối chiếu trực tiếp CD-ROM reset của DuckStation (mask1Fh), PSX-SPX
HCLRCTL/FIFO/BUSYSTS và DEV5.bit13. Chi tiết trong CD_BIOS_PROTOCOL_UPDATE.md.

## v18.2: response transport
PSX-SPX CDROM Drive, Response Timings: https://psx-spx.consoledev.net/cdromdrive/#cdrom-response-timings . The documented measured Nop average is 0xc4e1 clocks; Init is 0x13cce. We use those fixed delays for first responses, not a claim of cycle accuracy. Log (8) shows both existing replies ACKed; it does not prove why the BIOS GUI takes the menu route.

## v19.0: CPU/CD transfer review
References and limits are in CPU_CD_UPDATE_V19.md. Consulted PSX-SPX CPU load/store semantics, full GPR data pins for narrow DMA stores, CD host data requests and INT1 cadence, plus DuckStation's CPU state declaration. BFRD flush/reload and successive-load regressions fail before the patch. Retail verification is still outstanding.

## v19.1: Init completion event loss
The real log repeatedly emits INT2 one interpreter tick after ACK of INT3. Consulted PSX-SPX interrupt edge/ACK ordering and CD command response phases, and DuckStation's documented-in-source MINIMUM_INTERRUPT_DELAY=1000 constant. The independent project implementation applies a conservative 1000-tick source-low interval after an ACK transition. CD_INIT_IRQ_FIX_V191.md separates reproduced event loss from still-unverified retail boot.

## v20.0: MDEC reset and image-data path
Primary reference: https://psx-spx.consoledev.net/macroblockdecodermdec/ . The screenshot's 1F801824h write corresponds to the missing MDEC control register. Implemented the functional word-register/transform/DMA path and original fixtures; see MDEC_UPDATE_V20.md for accuracy limits and validation. No real GTA/FMVs were tested.

## v20.1: subword DMA register reads
User log proves LBU a0,2(v1) at PC 80127F40h reads DICR byte 2; v20.0's byte bus path lacked the DMA decoder. See DMA_SUBWORD_FIX_V201.md for the exact regression and PSX-SPX I/O-width reference. BIOS-to-game execution is now observed in the supplied log; gameplay/FMVs remain unverified.

## Version 21: DMA lanes, mirrors and CD audio

Primary references consulted online on 2026-10-26:
- https://psx-spx.consoledev.net/unpredictablethings/ — full GPR on SB/SH, shifted word DMA writes, BCR byte enables, CHCR mirrors, SPU odd-byte write ignore.
- https://psx-spx.consoledev.net/dmachannels/ — DICR W1C/gating and register definitions.
- https://psx-spx.consoledev.net/cdromdrive/ — Play, Mute/Demute, ATV/ADPCTL, filtering, reports and INT4.
- https://psx-spx.consoledev.net/cdromformat/ — XA block layout, coding, predictors and sample rates.
- https://psx-spx.consoledev.net/soundprocessingunitspu/ — CD input volume/enable, capture and independent mute behavior.

Current DuckStation dma.cpp/bus.cpp fetches were unavailable through the web service; no claim of source inspection for those files in this update. Prior DuckStation research remains documented above. All new implementation is original; no DuckStation code was copied. XA resampling is functional linear interpolation, not the hardware zigzag filter. Host adaptive playback is an original response to the supplied log's slow production/underruns.

## v21.1 — burst chopping

Supplied v21 log identifies CHCR=11400100 on CD DMA3, BCR=504, SW at80127fe4. Consulted PSX-SPX DMA channels online: bit8 burst chopping, bits16..18 word window, bits20..22 CPU window, chopping MADR/BCR progression, trigger versus busy. Implemented original functional scheduler for GPU/CD/SPU only, without pretending instruction ticks are cycle-accurate bus arbitration. See DMA_CHOPPING_FIX_V211.md.

## PSLab v25 PS2 research
Primary online sources: ps2tek hardware map and PCSX2 R5900.cpp / R5900OpcodeImpl.cpp. The newly written module implements only a CPU/ELF laboratory. No PCSX2 implementation was copied; no PS2 BIOS/game compatibility claim. See PSLAB_PS2_V25.md.

## v26 EE/GS registers
Consulted ps2tek map, PCSX2 R5900.cpp reset constants, GS.cpp CSR/IMR behavior and GS/GSRegs.h field definitions online. See EE_GS_REGISTERS_V26.md for supported operations and explicit latch-only limitations. No PCSX2 code copied.


## v27: PS2 GIF/GS and early BIOS MMIO
See PS2_GIF_BIOS_V27.md for primary-source URLs, implementation boundaries and the MCH register naming discrepancy in ps2tek. New code adds functional packet transport and initialization responses, not BIOS boot compatibility.


## v28: continuous runner and basic software GS
See PS2_RUN_GPU_V28.md for scope, primary-source references and the SLTI regression found in the user log. Raster coverage is functional, not GS pixel exact; BIOS boot remains incomplete.


## v28.1: SYNC barrier stop from BIOS trace
See PS2_SYNC_FIX_V281.md. GNU opcode tables identify 0000040f as SYNC.P; PS2SDK exposes sync.l/sync.p kernel macros. With synchronous stores and rendering, this functional EE core has no pending work to drain. Full pipeline/cache behavior is not modeled.


## v28.2: F500 boot store and write validation
See PS2_F500_FIX_V282.md. Uploaded trace fails on SW to B000F500. A narrowly scoped experimental 32-bit latch follows the legacy PCSX2 fallback storage/readback pattern; hardware side effects remain unmodeled. Memory-store validation now checks writability without issuing reads.


## v29: PS1 pitch-preserving output and low-contention presentation
See PS1_AUDIO_PERFORMANCE_V29.md for source references, original WSOLA implementation, WinMM/MMCSS changes, differential memory tests and limited synthetic benchmarks. No full-speed GTA1 or Windows audio validation is claimed from Linux core tests.


## v30: EE hardware, IRQs and normal GIF DMA
See PS2_HARDWARE_V30.md for source URLs and limitations. Original implementation adds word MMIO banks, timers, approximate blanking, interrupt entry, normal GIF DMA, branch-likely and packed register conversion. No PCSX2/Play! source was vendored. No commercial PS2 compatibility is claimed.


## v30.1: Wired stop in user screenshot
See PS2_WIRED_FIX_V301.md. Adds CP0 staging registers, Wired/Random and a 48-entry register/probe TLB bank. No virtual-memory translation or full MMU behavior is claimed. The screenshot failure opcode is tested directly.
