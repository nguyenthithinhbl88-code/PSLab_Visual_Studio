# PSLab v27 — GIF/GS và giao tiếp BIOS bước đầu

Bản cập nhật C++17 cho Visual Studio 2022. PS1 giữ backend hiện tại.
PS2 là phòng thử EE/MMIO: **chưa chạy được BIOS Sony đến Browser/logo và chưa chạy game PS2**.
Không kèm BIOS/game thương mại. ROM trong bài kiểm tra do dự án tự viết.

## Thay đổi

- Thêm đường truyền quadword CPU SQ → GIF FIFO PATH3 tại 0x10006000..0x10006fff, kể cả KSEG alias. SQ vẫn căn xuống biên 16 byte theo EE. Store 8 byte vào GIF không được coi như một gói 128 bit.
- Phân tích GIFtag, NLOOP, NREG (0 = 16), PRE/PRIM, danh sách descriptor và ranh giới packet. Giữ trạng thái qua nhiều lần ghi SQ.
- PACKED hỗ trợ PRIM, A+D và NOP. REGLIST hỗ trợ các descriptor GS hợp lệ trong dải 0..D và NOP; hai giá trị 64 bit mỗi quadword, bỏ padding khi số giá trị lẻ.
- Thêm **50 latch trạng thái** gồm PRIM, RGBAQ, ST/UV/XYZ/FOG; TEX0/TEX1/TEX2, CLAMP, XYOFFSET cho hai context; PRMODECONT, PRMODE, TEXCLUT, SCANMSK; MIPTBP1/2, TEXA, FOGCOL, TEXFLUSH; SCISSOR, ALPHA, DIMX, DTHE, COLCLAMP, TEST, PABE, FBA, FRAME, ZBUF; BITBLTBUF, TRXPOS, TRXREG, TRXDIR.
- SIGNAL và LABEL cập nhật ID theo mask vào SIGLBLID. SIGNAL/FINISH đặt cờ CSR; IMR điều khiển IRQ cục bộ và CSR W1C xóa cờ. NOP nhận cả mã A+D 0x0f và 0x7f.
- RDRAM: cổng lệnh MCH_RICM 0x1000f430, dữ liệu MCH_DRD 0x1000f440. Mô hình phản hồi khởi tạo đồng bộ: liệt kê hai thiết bị, các truy vấn SA 0x23/0x24/0x40, reset bộ đếm thiết bị. Chưa mô phỏng độ trễ vật lý.
- KPUTCHAR 0x1000f180 nhận store byte, giữ 16 KiB cuối; lọc byte điều khiển ngoài newline/tab/carriage return thành dấu chấm để log dễ đọc.
- Giữ nguyên lỗi đầu tiên, PC lỗi và 64 lệnh thử chạy gần nhất, kèm opcode/delay-slot. Sau lỗi cần Reset hoặc Load lại; các lần bấm Run không ghi đè nguyên nhân đầu tiên.
- Reset PC trở về entry của ELF/BIOS đang nạp thay vì luôn 0x00100000. RAM vẫn được giữ khi Reset; Load ELF nạp RAM mới như trước.
- UI PS2: thêm GIF demo, Run 100k, Save log; bố trí hai hàng nút. CLI thêm --bios, --gif-demo, --steps và xuất diagnostic kể cả khi dừng lỗi.

Các thanh ghi vẽ mới hiện **lưu giá trị để quan sát**, chưa thực hiện rasterization, texture sampling, TEX2/TEX0 alias side effects hay đầy đủ reserved-bit masks. BITBLTBUF/TRXPOS/TRXREG/TRXDIR chưa thực hiện truyền VRAM. FINISH tức thời vì chưa có công việc vẽ đang chờ. IRQ GS chưa nối EE INTC.

## Thử trên Windows

1. Giải nén vào thư mục mới, mở thư mục có CMakeLists.txt bằng Visual Studio 2022 với Desktop development with C++.
2. Chọn windows-release, Build target pslab, chạy pslab.exe. Tiêu đề phải có v27.
3. Tools → PS2 EE laboratory → GIF demo → Run 1k.
4. Kiểm tra `FRAME_1 [4c]=0000000000010001`, `GIF PATH3 qwords=2 packets=1 remaining=0 stopped=0`. Đây là kiểm tra đường truyền thanh ghi, không có hình vẽ.
5. Load BIOS (test) → chọn BIOS PS2 4 MiB của bạn → Run 100k. Nếu dừng, bấm Save log. Reset PC đưa về 0xbfc00000 và xóa lỗi/log đang giữ.
6. Gửi file log có First stop, PC, trace và KPUTCHAR để xác định phần CPU/MMIO còn thiếu.

CLI target pslab_ps2:

```powershell
.\pslab_ps2.exe --gif-demo --steps 1000
.\pslab_ps2.exe examples\ps2_gif_demo.elf --steps 1000
.\pslab_ps2.exe --bios D:\BIOS\your_ps2.bin --steps 100000 *> ps2_log.txt
```

--steps nhận 0..10000000. Mặc định 1000. Run 100k chạy một lượt hữu hạn trên UI thread, chưa có worker chạy liên tục.

## Kiểm chứng và giới hạn

- 38 bộ kiểm tra core được chạy bằng GCC C++17, -O2 -Wall -Wextra -Werror; gồm các kiểm tra hồi quy PS1 và ba bộ PS2.
- Bộ mới kiểm tra CPU→FIFO→GS, toàn bộ 50 latch, packet nhiều kiểu, padding/NREG/PRE, mask ID/CSR/IMR, lỗi không hỗ trợ, BIOS reset-vector với ROM tự viết, RDRAM/KPUTCHAR và giữ lỗi/trace.
- Bộ GIF/BIOS mới được chạy thêm AddressSanitizer + UndefinedBehaviorSanitizer, không phát hiện lỗi. LeakSanitizer bị tắt trong môi trường chạy này.
- ELF ví dụ do tools/make_ps2_gif_demo.py tạo, được chạy qua CLI để đối chiếu FRAME_1 và bộ đếm packet.
- Chưa build/chạy Win32 bằng MSVC tại môi trường này; cần kiểm tra UI trong Visual Studio trên máy Windows.
- Chưa có GS renderer/VRAM, DMA GIF, VIF/VU, IOP/SIF, CDVD, SPU2, TLB/exception/cache đầy đủ. EE chưa đủ bộ lệnh, Count không phải clock thật.
- IMAGE/HWREG, PACKED khác PRIM/A+D/NOP, hoặc SIGNAL thứ hai khi cờ còn pending sẽ dừng rõ ràng. GIF lỗi cần reset, không âm thầm bỏ dữ liệu. Một packet REGLIST có thể đã áp dụng giá trị đầu trước khi giá trị thứ hai gây lỗi.
- Không có bằng chứng boot BIOS Sony thật hay tương thích game PS2 từ bản này.

## Nguồn kỹ thuật

Viết logic của dự án từ tài liệu/đối chiếu thanh ghi, không tích hợp core PCSX2.

- PS2SDK GS register definitions: https://ps2dev.github.io/ps2sdk/gs__gp_8h_source.html
- PS2SDK GIFtag layout: https://ps2dev.github.io/ps2sdk/gif__tags_8h.html
- PCSX2 GS register IDs: https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS/GSRegs.h
- PCSX2 FIFO mapping: https://github.com/PCSX2/pcsx2/blob/master/pcsx2/FiFo.cpp
- PCSX2 GS privileged/event behavior: https://github.com/PCSX2/pcsx2/blob/master/pcsx2/GS.cpp
- ps2tek RDRAM initialization and reset-vector: https://psi-rockin.github.io/ps2tek/index.html

Lưu ý tài liệu: bảng địa chỉ đầu trang ps2tek ghi ngược tên hai cổng RDRAM so với pseudocode của chính trang. Bản này dùng địa chỉ trong pseudocode và định danh MCH_RICM=F430 của PCSX2 Hw.h; không dùng tên ở bảng tóm tắt làm căn cứ.
