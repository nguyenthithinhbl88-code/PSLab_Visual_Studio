# PSLab v31.1 — bàn phím và chẩn đoán tay cầm

Người dùng báo không điều khiển được GTA2 tại menu trong v31. Ảnh đã xác nhận
game tới menu nhưng không cho biết phím bị mất phía Windows hay trong giao thức SIO.
Chưa có log lúc giữ phím để tái hiện nguyên nhân. Bản này bổ sung đường nhận phím
và cách kiểm tra trực tiếp; chưa thể khẳng định đã sửa hết lỗi điều khiển GTA2.

## Thay đổi

- Mỗi UI timer khi chạy: đọc bit 15 của GetAsyncKeyState cho các phím đã gán,
  đồng bộ trạng thái nhấn/nhả thay vì chỉ phụ thuộc KEYDOWN/KEYUP.
  Giữ xử lý sự kiện để phản hồi ngay, bỏ thu phím khi cửa sổ chính không foreground.
- Thanh dưới game hiển thị Focus, Last host, số poll SIO đã hoàn tất,
  số poll có nút được nhấn và last pressed reply.
- View → Test pad: hold Cross for 1 second; View → Test pad: hold Start for 1 second.
  Đây là tín hiệu nút qua SIO trong 1 giây, không nhảy PC hoặc bỏ qua mã game.
  Bài thử tự Play nếu đang pause, kết thúc khi hết thời gian, Stop hoặc mất foreground.
- Export log giữ last_host và last_pressed_reply sau khi Stop đã nhả nút;
  thêm bộ đếm, CTRL/MODE/BAUD/IRQ/phase và last_TX để tìm nơi mất tín hiệu.
  Các bộ đếm được reset khi khởi tạo/reset cả đối tượng SIO trong machine boot.

## Cách thử

1. Build Release từ gói v31.1; mở pslab.exe có tiêu đề v31.1.
2. Load BIOS → Open disc/CUE → Boot disc → Play / Resume.
3. View → Keyboard: Arrows + Z/X/A/S để trở về bộ chuẩn.
4. Giữ Z khoảng nửa giây. Last host phải thành BFFF (bit Cross=0), rồi thử Enter.
5. Nếu không phản ứng, thử View → Test pad: hold Cross for 1 second.
   Nếu vẫn không qua menu, Export log và gửi kèm ảnh thanh trạng thái bên dưới game.

## Đọc kết quả

- Last host không đổi khi giữ phím: kiểm tra focus, bộ phím hoặc đường nhận phím Windows.
- Last host thay đổi, SIO polls không tăng: game chưa hoàn thành việc đọc tay cầm.
- Last pressed reply=BFFF hoặc FFF7: core đã trả Cross hoặc Start trong một gói hoàn chỉnh.
  Điều này chưa bảo đảm driver/game đã chấp nhận gói; log IRQ/CTRL giúp kiểm tra tiếp.

Microsoft: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate
Chỉ dùng bit cao thể hiện trạng thái đang giữ; không dùng bit thấp vốn không đáng tin cậy.
Foreground: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getforegroundwindow

Test core kiểm tra mất KEYDOWN, mất KEYUP, bỏ thu phím ở nền, gói reply và giữ bằng chứng
sau nhả phím. Môi trường Linux chưa chạy/build Win32/MSVC, nên cần kiểm tra trên máy người dùng.
Không thay đổi timing SIO/CPU/GPU/CD trong hotfix; tránh đoán nguyên nhân treo tay cầm từ ảnh menu.
