# PSLab v30.1 — sửa COP0 Wired trong ảnh lỗi

Ảnh v30 cho thấy core dừng sau 30 lệnh tại PC=BFC00854, opcode=40833000. Đây là MTC0 r3,Wired (COP0 register 6), chưa nằm trong whitelist cũ. Màn hình GS vẫn báo display circuit disabled.

## Thay đổi

- Thêm Wired và Random; reset Random=47, Wired=0. Wired đặt giới hạn dưới và khởi động lại Random; Random giảm theo lệnh đã thực thi, wrap trong khoảng Wired..47.
- Bổ sung Index, EntryLo0/1, Context, PageMask và EntryHi với các mask trường. Giữ bit chọn scratchpad của EntryLo0. Random, BadVAddr, PRId và BadPAddr không cho phần mềm ghi.
- Bank 48 entry hỗ trợ TLBWI/TLBWR lưu, TLBR đọc lại, TLBP probe theo VPN/PageMask và ASID/Global. Global của entry đòi hỏi cả hai G bit; TLBP miss đặt bit báo thất bại của Index.
- Thêm TagLo/TagHi dưới dạng latch phục vụ quan sát, không giả lập cache.
- Chẩn đoán in các COP0 mới và những entry đã ghi, kèm cảnh báo rõ về phạm vi mô hình.

**Đây là mô hình thanh ghi/lưu/probe TLB, chưa có MMU dịch địa chỉ hay kiểm tra quyền/refill/modified exception từ TLB.** Truy cập bộ nhớ tiếp tục dùng các alias phẳng của core. Không khẳng định BIOS/kernel đã sử dụng được virtual memory đầy đủ.

Giá trị Wired sau mask nằm trên 47 và Index ngoài bank 48 entry dừng lỗi rõ ràng. Các thanh ghi COP0 khác chưa hỗ trợ vẫn dừng; không bỏ qua toàn bộ opcode lạ. Entry chưa ghi được khởi tạo 0 cho kết quả thử nghiệm xác định, không đại diện nội dung TLB chưa khởi tạo trên máy thật. Nhiều entry cùng match chọn entry đầu tiên; chưa mô phỏng tình huống lỗi multiple-hit. Privilege/cache/timing vẫn chưa đầy đủ.

## Kiểm chứng

- Dùng đúng PC/opcode lỗi trong ảnh với r3=0: một bước đi từ BFC00854 đến BFC00858, Wired=0, Random=47, không first_fault. Đây là kiểm tra lệnh lỗi, **không phải phát lại toàn bộ BIOS từ ảnh**.
- Random bounds/wrap/read-only, các lệnh lưu/đọc/probe TLB, ASID/global/PageMask, mask stage và guard index.
- Tổng 47 bộ kiểm tra core PS1/PS2 bằng GCC C++17 -O2 -Wall -Wextra -Werror.
- Bộ COP0/TLB mới chạy ASan/UBSan; LeakSanitizer tắt trong môi trường này.
- Chưa có BIOS thật để kiểm tra điểm chạy tiếp theo, chưa build/chạy Win32 bằng MSVC ở đây. Phần PS1 v29 và hardware/DMA/GIF v30 được giữ lại.

## Thử lại

Giải nén thư mục mới → build Release target pslab → tiêu đề phải là **v30.1** → Load BIOS (test) → Run continuously. Nếu dừng tiếp, bấm Save log và gửi file text để có đầy đủ opcode/thanh ghi/trace.

Không có bằng chứng boot tới Browser/logo hoặc chơi game PS2 hoàn chỉnh từ hotfix này.

Nguồn đối chiếu thanh ghi và bank 48 entry:
https://psi-rockin.github.io/ps2tek/index.html (EE COP0 Memory Management, Index/Random/EntryLo/PageMask/Wired/EntryHi).

Logic mới do dự án viết, không ghép core của giả lập khác.
