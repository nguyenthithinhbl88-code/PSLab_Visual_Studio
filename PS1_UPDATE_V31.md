# PSLab v31 — cập nhật PS1

Mục tiêu: cải thiện điều khiển, CPU/GPU và đọc dữ liệu liên tục để tiến tới
GTA 2 qua intro và Gran Turismo 2 vào màn đua. Chưa có BIOS/ảnh đĩa thương mại
hoặc log GTA2/GT2 trong môi trường kiểm tra, nên chưa xác nhận hai mốc đó.
Đây là bản vá core dùng chung; không vá địa chỉ RAM riêng của game, không ép bỏ intro.

## Những thay đổi thực tế

- Bàn phím: hai preset trong menu View, lưu lựa chọn vào pslab.ini cạnh exe,
  tùy chọn trung hòa hai hướng đối nhau; nhả phím khi mất focus hoặc đổi preset.
  Có thể thay Key0..Key15 bằng mã virtual-key thập phân trong INI khi đóng app.
  Phím trùng, ngoài phạm vi, Alt, Escape và L3/R3 bị từ chối. Key=0 bỏ gán.
  Chọn preset trong menu xóa các override và lưu lại bộ chuẩn.
- SIO0: ACK có độ trễ baud*factor*8+150 instruction ticks và xung 100 ticks;
  chỉ byte chưa kết thúc gói mới có ACK. IRQ7 đến sau khi BIOS xóa ngắt cũ.
  Không xóa IRQ khi DSR còn asserted; đổi cổng, deselect hoặc reset hủy ACK chờ.
  Sau byte cuối, pad chờ /CS được nhả trước khi nhận gói tiếp.
  Log có số poll, số ACK, mức ACK và countdown.
- CPU: kiểm tra KUc khi fetch/load/store ngoài KUSEG, kể cả byte/unaligned
  và store khi cache isolated. Giữ kết quả load trước khi trap; EPC/BD được kiểm tra.
  COP0 trong user mode cần CU0; các thanh ghi COP0 không tồn tại phát RI;
  LWC0/SWC0 phát CpU0; giữ ưu tiên CU2 trước kiểm tra địa chỉ COP2.
- GPU: trả phiên bản v2 qua query 7, trả 0 qua query 8; sửa mask index 4 bit,
  command GP1 mirror 6 bit. GP1(09h) không còn làm core dừng; chọn vùng texture
  chip VRAM thứ hai không tồn tại trên mô hình retail sẽ trả texel trong suốt.
  Thêm đường đổi RGB opaque nhanh, tính cạnh tăng dần cho tam giác không texture;
  giữ đường tính cạnh cũ cho tam giác có texture theo kết quả benchmark.
- CD-ROM: Setloc chỉ lưu target, không đổi LBA của luồng đang đọc.
  ReadN/ReadS không có Setloc mới sẽ giữ FIFO, âm XA và nhịp đọc đang chạy.
  Read/Seek/Play áp dụng target khi cần; Init xóa target.
  Pause ACK giữ trạng thái Read cũ và completion trả trạng thái đã dừng.
  Log thể hiện target và setloc_pending để phân biệt chờ seek với chờ dữ liệu.

## Phím

| Nút PS1 | Preset mặc định | Preset WASD |
|---|---|---|
| D-pad | Mũi tên | W/A/S/D |
| Cross | Z | J |
| Circle | X | K |
| Square | A | U |
| Triangle | S | I |
| Start | Enter | Enter |
| Select | Shift | Tab |
| L1 / R1 | Q / W | Q / E |
| L2 / R2 | E / R | 1 / 3 |

View → Keyboard để đổi preset. Chọn Keyboard controls để xem hướng dẫn.
Đây là digital pad ID 41h/5Ah, chưa analog hoặc rung.
INI override dùng thứ tự bit PS1: Select=0, Start=3, Up=4, Right=5, Down=6,
Left=7, L2=8, R2=9, L1=10, R1=11, Triangle=12, Circle=13, Cross=14, Square=15.
Ví dụ `Key14=66` đổi Cross thành B. Bảng/hint hiển thị preset, không mô tả override tùy chỉnh.

## Build và thử GTA 2 / Gran Turismo 2

1. Giải nén vào thư mục mới, mở thư mục chứa CMakeLists.txt bằng Visual Studio 2022.
2. Chọn windows-release, Build, đặt pslab.exe làm Startup Item. Tiêu đề v31.
3. Load BIOS → Open disc → chọn CUE tham chiếu đầy đủ các BIN → Boot disc → Play / Resume.
4. Bấm vào màn hình game. Thử Enter (Start) hoặc Cross để đi tiếp nếu intro cho phép bỏ qua.
   Start ở đây là tín hiệu tay cầm thật trong mô hình SIO, không phải nút hack bỏ video.
5. Với GT2, thử riêng ảnh đĩa Arcade hoặc Simulation có sẵn; ghi rõ đĩa, vùng và revision.
   Sau menu cần thử vào một cuộc đua, lái xe và nghe âm thanh để xác nhận in-game.
6. Nếu vẫn kẹt: Export log tại điểm kẹt, chờ khoảng 5 giây khi vẫn Play, Export log lần hai
   sang tên khác. Hai log giúp so PC, CD sectors, MDEC, GPU và pad polls.
   Gửi cả hai cùng ảnh màn hình và mã đĩa SLUS/SLES/SCUS/SCES.

Không chọn Open EXE/Direct homebrew để thay đường khởi động game thương mại.
Boot disc dùng BIOS để khởi tạo kernel. Chưa bổ sung memory card/save state trong bản này.

## Kiểm chứng và giới hạn

Xem VALIDATION_V31.txt và BENCHMARK_V31.txt. Benchmark nhân tạo ghi nhận
thời gian vẽ flat/Gouraud không texture giảm khoảng 58%/45%; phép thử texture
còn chậm hơn khoảng 5%. Đây không phải FPS của GTA2 hay GT2. So sánh raster dựa trên VRAM hoàn chỉnh,
không chỉ đếm số lệnh. Kiểm thử CD dùng đĩa homebrew tự tạo và giao thức core.

Chưa build bằng MSVC hoặc chạy giao diện/thiết bị âm thanh Windows trong môi trường này.
Chưa thử GTA2/GT2 thật, chưa đo FPS của hai game. Không thể coi vượt test core là đã vào game.
SIO vẫn nhận RX tức thời, chưa mô phỏng đầy đủ bộ dịch/FIFO/phép đo clock vật lý.
CD seek/rotation/buffer overrun còn gần đúng; cơ chế resume sau Pause chưa được hoàn thiện.
GPU/GTE/timing chưa chính xác tuyệt đối. Mô hình VRAM chỉ 1 MiB, không hỗ trợ VRAM mở rộng thực.
SubQ đang tổng hợp từ track; chưa hỗ trợ SBI/SUB và LibCrypt. Bản game cần dữ liệu đó vẫn có thể treo.
Các nguồn đối chiếu nằm trong RESEARCH.md; phần bổ sung viết mới, không nhập core giả lập khác.
