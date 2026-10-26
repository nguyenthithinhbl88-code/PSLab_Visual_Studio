# PSLab v32.1 — PADDUW tại BFC00874

Log v32 của người dùng dừng sau 38 instruction tại BFC00874 với opcode 7000D428.
Đây là PADDUW r26,r0,r0, không phải lỗi GS. GS writes/GIF quadwords vẫn bằng 0.
Lệnh này phải ghi cả 128 bit của r26 thành 0, không chỉ 64 bit thấp.

Đã thêm nhóm unsigned saturation MMI1:

- PADDUW / PSUBUW: 4 lane 32 bit.
- PADDUH / PSUBUH: 8 lane 16 bit.
- PADDUB / PSUBUB: 16 lane 8 bit.

Cộng vượt giới hạn giữ giá trị cực đại của từng lane; trừ dưới 0 trả 0.
Các lane độc lập, thanh ghi đích trùng nguồn được xử lý từ bản chụp nguồn,
và đích r0 không bị thay đổi.

Test mới dùng đúng 39 instruction đã xuất hiện trong log (gồm instruction làm core dừng),
đặt vào các địa chỉ ROM tương ứng. Sau bản vá, replay đến BFC00878, 39 instruction,
không có first_fault; TLB/COP0 trước đó vẫn đúng và r26 được xóa đủ 128 bit.
Ngoài ra có 16800 phép kiểm tra lane theo dữ liệu có seed, đích trùng nguồn và r0.
Đây là replay đoạn log, không phải kiểm tra toàn bộ BIOS Sony.

Giải nén và build Release, chạy pslab.exe có tiêu đề v32.1.
Tools → PS2 EE laboratory → Load BIOS (test) → chọn BIOS của bạn → Run continuously.
Nếu dừng tiếp, Save log ngay trước khi Reset hoặc chọn demo.

Nguồn đối chiếu:
https://sources.debian.org/src/binutils/2.43.1-5/opcodes/mips-opc.c
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/MMI.cpp
https://psi-rockin.github.io/ps2tek/index.html

Chưa xác nhận boot hoàn chỉnh BIOS PS2. Không suy ra instruction kế tiếp trong BIOS
từ đoạn log bị cắt, và không ép bỏ qua các lỗi chưa được tái hiện.
