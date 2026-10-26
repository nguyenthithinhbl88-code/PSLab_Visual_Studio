# PSLab v32.2 — đọc BF803204 khi khởi động

Log v32.1 đã chạy 70067 instruction, vượt PADDUW. Điểm dừng mới:

```text
PC=9FC42B2C
opcode=90430000 (LBU r3,0(r2))
r2=FFFFFFFFBF803204
First stop: PS2 unmapped read @ 0xBF803204
```

BF803204 tương ứng địa chỉ vật lý 1F803204, nằm trong IOP hardware page 3
mà EE BIOS có thể truy cập. Chưa tìm được tài liệu xác định tên/chức năng riêng
của byte này. Không gán tên thiết bị hoặc bit ready chưa được kiểm chứng.

Đối chiếu PCSX2 1.6:

- Memory.cpp ánh xạ 1F803000..1F803FFF tới các handler IOP cho EE.
- IopHwRead.cpp trả backing byte tại page 3, ngoại trừ byte 1F803100.
- IopHw.cpp xóa backing IOP hardware khi reset.
- IopHwWrite.cpp lưu byte được ghi vào backing byte.

Bản vá chỉ bổ sung latch byte 1F803204, khởi tạo 0, dùng chung qua
physical/KSEG0/KSEG1. Có bộ đếm đọc/ghi trong log.
Đây là mô hình tương thích hẹp dựa trên cách PCSX2 xử lý backing storage;
chưa xác minh đặc tính phần cứng thật, chưa chạy IOP hoặc tạo handshake ready.
Các địa chỉ lân cận và độ rộng truy cập ngoài 1 byte vẫn báo lỗi.

Kiểm tra hồi quy phát lại đủ 64 instruction trong phần cuối log. Giá trị đầu
của r3 được suy ra từ số vòng countdown được ghi, vì log không có snapshot
thanh ghi tại đầu trace. Mọi PC và delay slot đều được đối chiếu.
Sau LBU, replay tới 9FC42B30 và r3.low=0; high half của GPR được giữ.
Đây là replay phần đuôi log, không phải chạy lại toàn bộ 70068 instruction BIOS.

Giải nén vào thư mục mới, Build Release, kiểm tra tiêu đề v32.2.
Tools → PS2 EE laboratory → Load BIOS (test) → chọn BIOS → Run continuously.
Nếu dừng ở vị trí khác, Save log trước khi Reset hoặc chọn demo.

Chưa xác nhận BIOS Sony boot hoàn chỉnh. Không có BIOS thật trong môi trường
kiểm tra và chưa chạy giao diện/MSVC trên Windows.

Nguồn:
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/Memory.cpp
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/ps2/Iop/IopHwRead.cpp
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/ps2/Iop/IopHwWrite.cpp
https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/IopHw.cpp
