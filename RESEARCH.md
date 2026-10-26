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


## PS1 v31 — đối chiếu tài liệu ngày 2026-10-26

- https://psx-spx.consoledev.net/controllersandmemorycards/ — digital pad ID 41/5A, active-low bits,
  ACK không phát sau byte cuối; BIOS xóa IRQ cũ sau khoảng 100 clocks nên ACK không được tức thời.
- https://psx-spx.consoledev.net/serialinterfacessio/ — DSR/IRQ, baud factor, CTRL acknowledgement.
- https://psx-spx.consoledev.net/cpuspecifications/ — KUc, AdEL/AdES, COP0 CU0,
  thanh ghi không tồn tại và LWC0/SWC0, ngoại lệ cho garbage registers.
- https://psx-spx.consoledev.net/graphicsprocessingunitgpu/ — GP1 command mirrors,
  GPU v2 query index mask 0xF, query 7/8, GP1(09h) và texture Y>=512.
- https://psx-spx.consoledev.net/cdromdrive/ — Setloc không đổi luồng đang đọc;
  Read lặp khi đang đọc tiếp tục luồng; Pause phản hồi trạng thái trước/sau.
- https://psx-spx.consoledev.net/dev/ps1/cdr/cdromfileformats/diskimages/ — SBI/SubQ.
  Chỉ nghiên cứu phạm vi; v31 chưa triển khai SBI hoặc LibCrypt.

Đã thử mở DuckStation digital_controller.cpp và LICENSE qua trình duyệt nhưng không lấy được nội dung.
Không nhập hoặc copy/paste mã nguồn DuckStation vào bản này. Phần triển khai mới viết tại đây từ tài liệu;
file tham chiếu raster trong test giữ lại thuật toán PSLab v30.1 để kiểm tra tối ưu.
Không có cơ sở từ các nguồn trên để kết luận lỗi GTA2 của người dùng đã được tái hiện hoặc đã hết.


## PS1 v31.1 — nhận phím Windows

https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate
Bit cao cho biết đang giữ phím; không dùng bit thấp. Đồng bộ định kỳ khi app foreground
để phục hồi trạng thái nếu mất KEYDOWN/KEYUP.
https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getforegroundwindow
https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getancestor
Không lấy phím khi cửa sổ PS2 riêng hoặc app khác là foreground. Không đổi timing SIO
khi chưa có log cho thấy lỗi ở đó; bổ sung bộ đếm gói đã trả và last pressed reply.


## PS2 v32 — CPU, DMAC source chain và GS Gouraud

- https://psi-rockin.github.io/ps2tek/index.html — bảng opcode MMI/BC0/EI/DI,
  DMAtag ID/QWC/ADDR/IRQ/PCE, CALL/RET/ASR/ASP, resume từ CHCR.TAG,
  nguồn RAM/scratchpad, ngắt DMAC và tham số GS.
- https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/MMI.cpp — đối chiếu
  thứ tự PCPYLD/PCPYUD, HI1/LO1, sign-extension và QFSRV.
- https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/COP0.cpp — đối chiếu
  quyền EI/DI qua EDI/EXL/ERL/KSU.
- https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/R5900OpcodeImpl.cpp —
  đối chiếu MFSA zero extension, MTSA, MTSAB/MTSAH.

Phần bổ sung là triển khai mới trong PSLab, không vendoring/copy nguyên core PCSX2.
GS Gouraud hiện là nội suy chức năng, không tuyên bố raster chính xác phần cứng.
ROM thử đi kèm chỉ chứa mã test do dự án viết; không chứa BIOS Sony hoặc dữ liệu game.
Không dùng ROM thử để suy ra BIOS Sony đã boot thành công.


## PS2 v32.1 — PADDUW log regression

https://sources.debian.org/src/binutils/2.43.1-5/opcodes/mips-opc.c — encoding PADDUW base 70000428;
7000D428 is PADDUW r26,r0,r0.
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/MMI.cpp — unsigned per-lane saturation.
https://psi-rockin.github.io/ps2tek/index.html — MMI1 opcode table for PADDUW/H/B and PSUBUW/H/B.
Implementation written in PSLab; no imported emulator core. User-supplied log(4) supplies the
39 attempted instructions for replay, not any missing instructions beyond the reported fault.


## PS2 v32.2 — IOP page-3 bootstrap byte

https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/Memory.cpp
Maps EE physical page 1F803000 to IOP page-3 handlers for boot/debug accesses.
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/ps2/Iop/IopHwRead.cpp
Page-3 byte reads use backing storage except 1F803100 (which returns FF).
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/ps2/Iop/IopHwWrite.cpp
Byte writes update backing storage.
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/IopHw.cpp
psxHwReset clears the IOP hardware backing array.

The new PSLab model is deliberately limited to byte 1F803204 with reset=0
and read/write counters. No hardware name/bit meaning or IOP-ready handshake is
inferred. A supporting web search hit was an unverified third-party emulator log;
it was not used as implementation evidence. Replay comes from user's log(5).

## v32.3: IOP page-1 halfword storage at 1F801472
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/ps2/Iop/IopHwWrite.cpp
_HwWrite_16or32_Page1 uses backing storage in its default case. There is no
explicit 1472 case. PSLab implements only the logged halfword address as
storage, without claiming real hardware side effects or widening all IOP pages.
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/ps2/Iop/IopHwRead.cpp
Default page-1 read is backed by hardware storage. Real meaning of 1472 remains
unverified. Regression input comes from the user's log(6), not an emulator ROM.

## v33 grouped IOP bootstrap register and memory coverage
PS2SDK SSBUS tables (retrieved upstream directly using HTTPS):
https://raw.githubusercontent.com/ps2dev/ps2sdk/master/iop/system/ssbusc/src/ssbusc.c
PS2SDK register identifiers:
https://israpps.github.io/ps2sdk/iop__regs_8h_source.html
ps2tek IOP interrupts/DMA; EE IOP RAM and VU code/data physical map:
https://psi-rockin.github.io/ps2tek/index.html
See PS2_IOP_REGISTER_UPDATE_V33.md for implemented behavior versus storage-only
models. No IOP execution or DMA completion is synthesized. Configuration reset
zeros (other than documented DPCR) are model policy, not verified silicon data.

## v34 EE SIO and VIF bootstrap groups
PS2SDK SIO register definitions and SDK init/polling behavior:
https://ps2dev.github.io/ps2sdk/sio_8h.html
https://ps2dev.github.io/ps2sdk/sio_8c_source.html
PCSX2 SIO empty status and TX width handling, also inspected current upstream:
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwRead.cpp
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwWrite.cpp
https://raw.githubusercontent.com/PCSX2/pcsx2/master/pcsx2/HwRead.cpp
https://raw.githubusercontent.com/PCSX2/pcsx2/master/pcsx2/HwWrite.cpp
ps2tek VIF I/O register map, permissions and configuration command format:
https://psi-rockin.github.io/ps2tek/index.html
See PS2_SERIAL_VIF_UPDATE_V34.md for precise scope. Multi-byte TX, empty RX,
reset values and FBRST read are model policies, not silicon measurements.

## v35 COP1 raw register and memory bootstrap
https://docs.alexrp.com/mips/ee.pdf
EE Core User's Manual: 32 FPRs, FCR0/FCR31, fixed/control fields and FPU limits.
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/FPU.cpp
MFC1 sign extension, MTC1 raw bits, MOV.S and LWC1/SWC1 bus transfers.
https://psi-rockin.github.io/ps2tek/index.html
EE opcode/COP1 decoding and branch types. See PS2_COP1_BOOTSTRAP_V35.md for
reserved-index/reset policies and exact limits. No arithmetic engine added.

## v36 memory bootstrap, DMA/VIF upload and GS alpha
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwRead.cpp
F410/RICM zero-read reference; RDRAM response values.
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwWrite.cpp
Default backing writes and RICM INIT/busy behavior; unidentified registers
remain explicitly experimental, with no claimed hardware bit meanings.
https://raw.githubusercontent.com/PCSX2/pcsx2/master/pcsx2/Hw.h
MCH block range F400..F500, RICM/DRD names.
https://psi-rockin.github.io/ps2tek/index.html
Normal DMA progress, VIF registers/MPG/UNPACK, GS ALPHA/COLCLAMP/TEST.
See the v36 register map and implementation notes for exact support/limits.
No third-party emulator code copied. Forum memory-init traces encountered in
search were corroborative only, not the source of register bit semantics.

## v37 MCH narrow accesses and boot execution cost
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwRead.cpp
_hwRead8/_hwRead16 extract lanes from one aligned word-handler result.
https://psi-rockin.github.io/ps2tek/index.html
COP0 Count cycle counter and GS display configuration. No cycle skip added.
https://pcsx2.net/docs/advanced/psx/
EE/IOP system bus relationships. Real BIOS menu boot is still unconfirmed.
See PS2_BOOT_PROGRESS_V37.md for partial-write policy, performance changes,
validation limits and benchmark provenance. Original code, no external copy.

## v38 CACHE bootstrap maintenance
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/Cache.cpp
Identifies 07 IXIN, 0C BFH, 14 DXWBIN, 16 DXIN, 18 DHWBIN, 1A DHIN,
1C DHWOIN. Reference IXIN/BFH handling has no extra effect when those caches
are not present. PSLab's D-cache maintenance is explicitly coherent-memory
policy, not copied full cache/writeback emulation. Tag/data operations and
cache/TLB exceptions remain unsupported. See PS2_CACHE_BOOT_FIX_V38.md.
