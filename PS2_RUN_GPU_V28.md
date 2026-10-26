# PSLab v28 — Run liên tục, màn hình GS và GPU cơ bản

C++17 / Win32 / Visual Studio 2022. Bản này giữ core PS1 và bổ sung phần PS2.
**PS2 vẫn chưa boot BIOS Sony hoàn chỉnh hoặc chạy game thương mại.**

## Lỗi cụ thể trong log bạn gửi

Log dừng sau 2 lệnh tại PC=0xbfc00008, opcode=0x2b410059. Đây là SLTI r1,r26,0x59; r26 đang chứa PRId=0x2e20. Core v27 thiếu SLTI và đã giữ lỗi đầu tiên, vì vậy bấm Run lại không chạy thêm được.

v28 bổ sung SLTI với so sánh signed 64 bit và immediate sign-extend. Bài kiểm tra phát lại MFC0 PRId, NOP, SLTI từ log để xác nhận PC đi đến 0xbfc0000c và r1=0. Không có BIOS Sony thật tại môi trường phát triển; chưa biết lệnh/MMIO chưa hỗ trợ tiếp theo trong BIOS của bạn.

## Không cần bấm Run 100k nhiều lần

1. Build Release target **pslab**, chạy pslab.exe. Tiêu đề chính phải có v28.
2. Tools → PS2 EE laboratory.
3. Load BIOS (test) → chọn BIOS PS2 4 MiB → **Run continuously**.
4. Nút đổi thành **Pause**. Bấm để tạm dừng; bấm lại để tiếp tục từ PC hiện tại.
5. **Stop** dừng chạy và giữ PC. **Reset PC** trở về entry của BIOS/ELF, xóa lỗi/trace và giữ RAM; sau đó bấm Run nếu muốn khởi động lại.
6. Khi gặp lệnh/MMIO chưa hỗ trợ, Run tự dừng và giữ nguyên lỗi đầu tiên. Bấm **Save log** để gửi chẩn đoán.

Run dùng WM_TIMER 16 ms, mỗi lượt tối đa 10000 lệnh và kiểm tra ngân sách khoảng 4 ms sau mỗi 64 lệnh. Cùng UI thread, không có truy cập core đồng thời hoặc thread chạy còn sót sau khi đóng cửa sổ. Một lệnh vẽ GS lớn có thể vượt ngân sách lượt. Đây là bộ chạy hữu hạn từng lượt nối tiếp, chưa phải giả lập PS2 đúng tốc độ/cycle.

Mọi lệnh đổi chương trình, Step, Reset và hộp thoại đều tạm dừng Run trước khi thực hiện. Save log cũng tạm dừng để file chứa trạng thái nhất quán. Run tự giữ Pause khi gặp lỗi; cần Reset/Load lại trước khi thử tiếp.

## Màn hình PS2 GS riêng

- **GS Screen** mở cửa sổ framebuffer riêng, có thể di chuyển/phóng to. Đóng màn hình chỉ ẩn cửa sổ; bấm GS Screen để mở lại.
- Load BIOS hoặc GPU demo cũng mở màn hình. Đóng phòng thử PS2 hủy cửa sổ màn hình và dừng timer.
- Ảnh đọc từ VRAM theo PMODE, DISPFB1/2, DISPLAY1/2, DBX/DBY và MAGH/MAGV; BGCOLOR khi display circuit tắt. Ưu tiên circuit 1 nếu cả hai bật, chưa hòa trộn hai circuit, xử lý DX/DY hoặc interlace.
- Scanout hỗ trợ PSMCT32/PSMCT24; kích thước có giới hạn 1024x1024 để tránh cấp phát không kiểm soát. Các chế độ lớn hơn bị cắt về giới hạn. UI co ảnh giữ tỉ lệ pixel, không ép mọi độ phân giải sang 4:3.
- Nền đen hoặc thông báo display circuit disabled trong khi BIOS dừng sớm là bình thường. Không chèn ảnh/logo BIOS giả.

## GPU được thêm trong bản này

- VRAM GS **4 MiB**. Địa chỉ PSMCT32 theo page 64x32, block 8x8, column 8x2, wrap trong VRAM. PSMCT24 chia sẻ layout và giữ byte alpha cũ khi vẽ.
- Vertex kick XYZ2/XYZF2; XYZ3/XYZF3 cập nhật hàng đợi mà không vẽ. Hàng đợi điểm, line/list/strip, triangle/list/strip/fan và sprite.
- Bộ vẽ phần mềm cho hình không texture, màu phẳng: point, line, triangle, sprite. Áp dụng context 1/2, PRMODECONT/PRMODE, XYOFFSET, SCISSOR, FRAME, FBMSK và FBA.
- Pixel coverage và đường thẳng là xấp xỉ chức năng, chưa bảo đảm giống GS từng pixel/subpixel. Chưa Gouraud, texture, fog, alpha blend, Z buffer/test, alpha test, dithering đầy đủ hoặc AA. Nếu primitive dùng tính năng chưa hỗ trợ, bộ vẽ bỏ primitive đó và ghi `skipped` cùng `Renderer warning`; CPU tiếp tục để phục vụ chẩn đoán. Không âm thầm coi là vẽ thành công.
- Host→local upload PSMCT32 qua BITBLTBUF/TRXPOS/TRXREG/TRXDIR, GIF IMAGE/IMAGE2 và HWREG. Giữ tiến độ qua nhiều gói, bỏ phần padding khi đã đủ pixel. VRAM→host và local→local chưa hỗ trợ; các yêu cầu đó dừng lỗi rõ ràng.
- Reset GS xóa VRAM để thử nghiệm có kết quả xác định; đây không phải cam kết nội dung VRAM sau reset trên máy thật.
- Chưa DMA GIF, VIF/VU, IOP/SIF, CDVD, SPU2, timing/video interrupt thực. GS dùng CPU host để vẽ, chưa dùng Direct3D/Vulkan.

## Thử GPU ngay

Trong phòng thử PS2: **GPU demo → Run continuously**. Cửa sổ GS sẽ hiện nền tối, hai hình chữ nhật xanh, một tam giác cam, đường thẳng và điểm trắng. Đây là demo do dự án tự viết; CPU thực thi LQ/SQ chuyển packet GIF đến bộ vẽ.

Demo harness khởi tạo thanh ghi scanout; lệnh vẽ đi qua CPU/GIF. Sau khi hoàn tất, CPU chạy vòng lặp chờ nên ảnh đứng yên là bình thường. Bộ đếm chuẩn: `GS primitives=6`, `skipped=0`.

CLI:

```powershell
.\pslab_ps2.exe --gpu-demo --steps 1000
.\pslab_ps2.exe --bios D:\BIOS\your_ps2.bin --steps 100000 *> ps2_log.txt
```

CLI vẫn dùng ngân sách hữu hạn --steps; Run liên tục nằm trong UI Windows.

## Kiểm thử và nguồn

- Bộ mới: phát lại lỗi SLTI trong log, signed/immediate corner cases; địa chỉ VRAM cụ thể và ánh xạ đủ một page; mask/clipping/context; XYZ3; triangle coverage; IMAGE/HWREG/padding; scanout/crop/BGRA; CPU/GIF GPU demo.
- Tổng 39 bộ kiểm tra core PS1/PS2 bằng GCC C++17. Bộ render mới chạy thêm ASan/UBSan (tắt LeakSanitizer do môi trường).
- Kiểm tra ảnh scanout do core tạo. Chưa build/chạy Win32/MSVC ở môi trường phát triển này, nên cần kiểm chứng Run/Pause/Stop, resize và đóng/mở màn hình trên Windows.

Nguồn đối chiếu kỹ thuật, logic viết riêng trong dự án:

- https://psi-rockin.github.io/ps2tek/index.html — EE opcode, GS vertex/context/framebuffer/transfer.
- https://ps2dev.github.io/ps2sdk/gs__privileged_8h_source.html — thanh ghi scanout.
- https://ps2dev.github.io/ps2sdk/gs__gp_8h_source.html — GS register fields.
- https://gist.github.com/tellowkrinkle/bd6c6e1735cf5e03110ec57ddeea43a9 — mô tả hoán vị địa chỉ VRAM của tác giả công cụ GS swizzle visualizer; hiện thực bằng phép ghép bit, không tích hợp mã JS/công cụ này.
