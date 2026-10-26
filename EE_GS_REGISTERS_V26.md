# PSLab v26 — EE + GS registers

Tập trung phần PS2; không thay đổi lõi PS1. Chưa chạy BIOS/game PS2.

## CPU EE

- HI/LO với MFHI/MTHI/MFLO/MTLO, MULT/MULTU, DIV/DIVU; trường hợp chia0 và INT_MIN/-1 được xử lý rõ ràng. MULT cũng ghi rd khi rd khác0.
- Thêm shift32/64bit, variable shift32, MOVZ/MOVN, SLT, BLEZ/BGTZ.
- COP0 latches: BadVAddr8, Count9, Compare11, Status12, Cause13, EPC14, PRId15, Config16, ErrorEPC30. MFC0/MTC0, PRId/BadVAddr chỉ đọc ở đường MTC0.
- Reset PRId2e20, Config440, Status70400004 theo đối chiếu mã nguồn PCSX2.
- ERET chọn EPC hoặc ErrorEPC theo EXL/ERL và không chạy delay slot.
- Count tăng1 mỗi lệnh trong model. Compare chưa phát ngắt; Status/Cause/Config chủ yếu là latch, chưa mô hình privilege/cache/TLB/exception đầy đủ. Không tuyên bố CP0 cycle accurate.
- HI1/LO1, MMI/FPU/VU và nhiều lệnh EE vẫn chưa có.

## GS

Module ps2_gs xử lý19 thanh ghi privileged:
PMODE, SMODE1/2, SRFSH, SYNCH1/2, SYNCV, DISPFB1/2, DISPLAY1/2,
EXTBUF, EXTDATA, EXTWRITE, BGCOLOR, CSR, IMR, BUSDIR, SIGLBLID.

- MMIO từ12000000 và KSEG aliases, word32/doubleword64 căn chỉnh; halfword/byte và gap chưa hỗ trợ báo lỗi.
- Mask cơ bản PMODE/SMODE2/BGCOLOR/EXTWRITE/BUSDIR, IMR bits8..12 cùng reserved6000.
- CSR có5 cờ event write-one-to-clear, reset bit9 xóa các latch; FLUSH không có FIFO để xử lý. FIFO trạng thái chọn empty, chưa mô hình revision/ID của chip cụ thể.
- IRQ GS nội bộ được tính từ CSR/IMR nhưng CHƯA nối tới EE INTC/CPU.
- SIGLBLID chỉ đọc trong đường privileged; chưa có GIF SIGNAL/LABEL để cập nhật.
- Các CRTC/timing registers hiện lưu latch và cho đọc lại để debug; chưa mô hình đầy đủ reserved-bit/read-only/write-only/PLL/PCRTC của phần cứng.
- Chưa có GIF/VIF/VU, VRAM, framebuffer, rasterizer, texture hay video timing tự động. Thêm thanh ghi không đồng nghĩa đã có GPU renderer PS2.

## Thử

Build Release, mở pslab.exe có tiêu đề PSLab v26 - EE and GS Registers.
Tools → PS2 EE laboratory → CPU + GS demo → Run1k.
Kết quả: RAM[200]=23 (hex35), r10=2e20, BGCOLOR=bb55.
Toàn bộ thanh ghi GS, HI/LO và các COP0 được in trong panel, cuộn xuống để xem.

Hoặc Open ELF → examples/ps2_register_demo.elf → Run1k.
CLI: pslab_ps2 examples/ps2_register_demo.elf.
Tái tạo fixture: python tools/make_ps2_register_demo.py. Không có byte Sony/game.

## Kiểm chứng

37 bộ test lõi PASS GCC C++17 -O2 -Wall -Wextra -Werror.
ps2_register_tests PASS ASan/UBSan, leak detection tắt do giới hạn môi trường ptrace.
CLI ELF mẫu chạy1000lệnh, kiểm tra product/PRId/BGCOLOR chính xác như trên.
Test kiểm tra chia0/overflow, bảo toàn high64 GPR, COP0/ERET, signed shift,
GS lanes/aliases/masks/W1C/reset/IRQ nội bộ và địa chỉ/width không hỗ trợ.
UI Windows mới chưa build/chạy bằng Windows SDK tại môi trường này.

## Tra tài liệu trực tuyến ngày26/10/2026

- https://psi-rockin.github.io/ps2tek/index.html : EE/GS memory map và địa chỉ thanh ghi.
- https://github.com/PCSX2/pcsx2/blob/master/pcsx2/R5900.cpp : giá trị reset CP0.
- https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS.cpp : CSR W1C/reset và IMR.
- https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/GSRegs.h : bố trí bit GS.

Implementation tự viết, không sao chép implementation PCSX2.
