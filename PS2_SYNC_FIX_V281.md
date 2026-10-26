# PSLab v28.1 — sửa BIOS dừng ở SYNC.P

## Log thực tế

Bản v28 của người dùng đã vượt qua SLTI và chạy 12 lệnh, sau đó dừng tại:

```
PC=bfc0080c
First stop: PS2 EE SPECIAL not implemented @ 0x40f
COP0[16]=73003
GIF PATH3 qwords=0 packets=0
PMODE=0
```

0x0000040f là SYNC.P. Màn hình đen ở thời điểm này phù hợp với PMODE=0 và chưa có gói GIF; log chưa cho thấy lỗi ở bộ vẽ GS.

## Sửa trong core

- Thêm nhận dạng chính xác SYNC.L (0x0000000f) và SYNC.P (0x0000040f).
- Core hiện thực thi RAM store, GIF consumption và software rendering đồng bộ, hoàn tất ngay trong lệnh gọi. Vì chưa có store buffer, cache hoặc hàng đợi GPU chạy bất đồng bộ, barrier không cần chờ thêm trong mô hình hiện tại. Đây là lựa chọn của mô hình chức năng, không phải giả lập đầy đủ pipeline/cache/timing EE.
- Lệnh vẫn tăng PC, Instructions và COP0 Count theo cách hiện có; không thay đổi GPR.
- Không bỏ qua tùy tiện các lệnh SPECIAL chưa biết; lỗi đầu tiên vẫn được giữ.
- Giữ Run/Pause/Stop, cửa sổ GS và toàn bộ GPU cơ bản của v28.

## Kiểm chứng

Bài kiểm tra mới phát lại 13 cặp PC/opcode trong log, gồm nhánh/delay-slot, JR, MTC0 Config rồi SYNC.P. Kết quả sau lệnh cuối: PC=0xbfc00810, Next=0xbfc00814, Instructions=13, Config=0x73003, không có first_fault.

Kiểm tra thêm SYNC.L/P bảo toàn GPR, tăng bộ đếm và khả năng đọc lại store trước barrier. Lệnh SPECIAL khác chưa hỗ trợ vẫn dừng đúng PC.

Tổng 40 bộ kiểm tra core PS1/PS2 được chạy bằng GCC C++17 -O2 -Wall -Wextra -Werror. Chưa chạy BIOS Sony thật sau chỗ dừng này; chỉ có đoạn trace do người dùng cung cấp. Chưa build MSVC tại môi trường phát triển. Ảnh người dùng xác nhận UI v28 và tự dừng khi lỗi đã hoạt động trên Windows; các giới hạn PS2 trong PS2_RUN_GPU_V28.md vẫn áp dụng.

## Thử lại

Giải nén bản mới vào thư mục mới; build Release target pslab. Tiêu đề phải là **v28.1**. Load BIOS (test) → chọn BIOS → Run continuously. Nếu đã nạp BIOS, Reset PC rồi Run sẽ xóa lỗi cũ.

Nếu dừng tiếp, Save log để xác định lệnh/MMIO còn thiếu. Không thể khẳng định sẽ vào Browser/logo PS2 từ hotfix này.

## Nguồn đối chiếu

- GNU binutils opcode definitions (sync.p=0x0000040f, sync.l=0x0000000f): https://gnu.googlesource.com/binutils-gdb/+/refs/tags/gdb_7_5_1-2012-11-29-release/opcodes/mips-opc.c
- PS2SDK kernel barrier macros: https://github.com/ps2dev/ps2sdk/blob/master/ee/kernel/include/kernel.h
