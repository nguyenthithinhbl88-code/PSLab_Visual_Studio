# v23 — Keyboard Pad 1

Bấm vào vùng hình game, sau đó chơi bằng phím. Nút Keyboard Pad mở bảng hướng dẫn;
thanh dưới các nút luôn hiện mapping, debugger hiện PAD1 buttons để kiểm tra nút.

| Bàn phím | Tay cầm PS1 |
|---|---|
| Mũi tên | D-pad |
| Z | Cross (✕) |
| X | Circle (○) |
| A | Square (□) |
| S | Triangle (△) |
| Enter | Start |
| Shift | Select |
| Q / W | L1 / R1 |
| E / R | L2 / R2 |

Trong menu GTA1 dùng mũi tên và Z để chọn. Trong màn chơi, chức năng của mỗi nút
do game quyết định; có thể xem Controls trong GTA để biết nút lên xe/bắn/chạy.

Trước đây có digital SIO và polling GetAsyncKeyState, nhưng UI thiếu bảng phím.
Bản23 tách mapping vào KeyboardPad, bắt WM_KEYDOWN/WM_KEYUP ở message loop cho cả
các cửa sổ con, gửi trạng thái ngay vào PAD1 dưới mutex. Phím đã map được tiêu thụ
để Enter không vô tình kích hoạt nút Run/Reset đang focus. Giữ nhiều phím và key
repeat không tạo toggle sai. Alt-Tab/WM_ACTIVATEAPP, Pause và mở hướng dẫn nhả phím.
Không cần bật Caps Lock; mapping dùng virtual key Z/X/A/S theo Windows.

Kiểm thử mới đi qua từng nút, tổ hợp, key repeat, nhả/focus và giao tiếp SIO thật
1/42/00/00/00: ID41, sync5A, hai byte nút active-low, snapshot giữa giao dịch.
35 bộ test lõi PASS với GCC C++17 warnings-as-errors. Chưa build/chạy UI Windows
và thử bàn phím trong GTA thật tại môi trường này. Vẫn cần xác nhận trên máy bạn.

Đây là digital pad cổng1. Chưa có analog, rung, cổng2, remap hay tay cầm USB.
Các thay đổi âm thanh/hiệu năng v22 được giữ lại. Tiêu đề mới:
PS1 Lab v23 - Keyboard Pad.
