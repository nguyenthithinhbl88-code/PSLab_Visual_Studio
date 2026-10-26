# PSLab v25 — PS1 + PS2 EE laboratory

Ứng dụng đổi tên PSLab; target Windows là pslab, đầu ra pslab.exe.
PS1 giữ đường chạy BIOS/CD/GTA và các phần điều khiển, âm thanh trước đây.
Chọn Tools → PS2 EE laboratory (experimental) để mở cửa sổ CPU PS2 thử nghiệm.
Phiên PS1 sẽ Pause trước khi mở phòng EE; đóng phòng EE rồi Play để tiếp tục PS1.

## Đã lập trình

- Lõi EE riêng với32 thanh ghi lưu128bit, scalar32/64bit và LQ/SQ128bit.
- Một tập lệnh integer/load/store/branch/jump nhỏ, nhánh có delay slot; chưa đủ ISA.
- RAM32MiB, scratchpad16KiB, BIOS4MiB; aliases KSEG0/KSEG1 trực tiếp.
- ELF32 little-endian MIPS ET_EXEC: nạp PT_LOAD, zero BSS, kiểm tra bounds/entry,
  từ chối segment chồng lấn, lỗi validation không phá trạng thái trước đó.
- Step, Run1k hữu hạn, resetPC, demo gốc, nạp ELF và BIOS để thử ánh xạ reset.
- examples/ps2_ee_demo.elf do tools/make_ps2_demo.py tạo: tính7+5, ghi12 tạiRAM200.
- CLI pslab_ps2 [bare-metal.elf], mặc định demo gốc; chạy1000lệnh rồi xuất thanh ghi.

## Cách thử

Build x64 Release, chọn startup pslab.exe.
Tools → PS2 EE laboratory → Original demo → Run1k: RAM[200] hiện c (12 hệ thập phân).
Hoặc Open ELF → examples/ps2_ee_demo.elf → Run1k.
Load BIOS chỉ chấp nhận4MiB, ánh xạ và reset tới BFC00000; KHÔNG có khả năng boot BIOS.
Nạp homebrew thông thường gọi kernel/GS cũng sẽ dừng, vì chưa có các dịch vụ này.

## Chưa có

Đây chưa phải máy PS2 đầy đủ: không GS/GIF/VIF/VU0/VU1, IOP/SIF, SPU2, CDVD,
DMAC/timers/interrupts PS2, TLB/cache, FPU/MMI/COP0 hoặc exception kiến trúc hoàn chỉnh.
Uncached aliases20000000/30100000 chưa triển khai. Không syscalls/BIOS kernel, không
analog/rung/thẻ nhớ PS2. Chưa có ISO PS2, BIOS menu, FMV hay game PS2 thương mại.
Lỗi opcode/MMIO/alignment được báo bằng host exception thay vì giả vờ chạy tiếp.
Chưa mô hình clock/cycle hay đồng bộ các vi xử lý. ResetPC giữ RAM; ELF mới xóa RAM.
Lõi/loader là mã gốc của dự án, không sao chép implementation GPL của PCSX2.

## Kiểm thử

36 bộ test lõi PASS với GCC C++17 -O2 -Wall -Wextra -Werror.
ps2_ee_tests PASS ASan/UBSan (leak detection tắt do môi trường ptrace).
CLI chạy ELF mẫu1000lệnh, RAM[200]=c. Fixture tự tạo, không BIOS/game Sony.
UI Win32 mới chưa build/chạy bằng Windows SDK tại môi trường này.

## Tài liệu đã tra trực tuyến

- https://psi-rockin.github.io/ps2tek/index.html — tổng quan hệ thống, RAM/BIOS/scratchpad.
- https://github.com/PCSX2/pcsx2/blob/master/pcsx2/R5900.cpp — kết nối EE với các khối PS2.
- https://github.com/PCSX2/pcsx2/blob/master/pcsx2/R5900OpcodeImpl.cpp — đối chiếu hành vi scalar và LQ/SQ tự căn16byte; không chép code.

Các bước tiếp: mở rộng ISA/COP0/exception → IOP/SIF/DMAC → GIF/GS/VU → SPU2/CDVD → bootBIOS → kiểm chứng homebrew đồ họa → game.
