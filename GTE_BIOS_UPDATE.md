# Bản18 — COP2/GTE và giữ lỗi CPU gốc

## Kết luận từ log mới

Log lúc kẹt logo lần này khác log nạp EXE trực tiếp:

- Kernel đã khởi tạo; các vectorA0/B0/C0 có mã thật.
- 726.245.929 lệnh đã thực thi; BIOS gọiA0 khoảng39triệu lần.
- `Cause=00000428`: exception code10 (Reserved Instruction), IRQ pending IP2.
- `reserved_exceptions=1`, `last_reserved_pc=8004b70c`, `EPC=8004b70c`.
- BIOS đang gọi `A0(40h)`, bảng hàm trỏ đến vòng lặp ởROM `bfc0d8e0`.
  Đây là SystemErrorUnresolvedException, không phải vòng đang chờ nạp game bình thường.
- CD mới nhận2lệnh, chưa đọc sector; GPU vẫn giữ hình Sony đã vẽ.
- 22,53MIPS /39,55VideoHz chứng tỏ CPU có thực thi; MIPS tăng không đồng nghĩa game tiến triển.

**Đã xác định:** sau một lệnh gây Reserved Instruction, BIOS đi vào vòng lỗi vô hạn.
**Chưa xác định được opcode của lần lỗi trên:** bản17.1 chỉ lưuPC và32opcode gần nhất;
trace lúc gây lỗi đã bị hàng triệu lần lặp ghi đè. Không gọi đây là xác nhận lỗi COP2.
Mã CPU cũ chưa xử lý COP2/LWC2/SWC2, trong khi SR lúc lỗi bậtCU2; đó là thiếu sót
cụ thể cần bổ sung, và là một giả thuyết hợp lý cho exception này.

## Đã thêm

- MFC2/CFC2, MTC2/CTC2; MFC2/CFC2 dùng cùng load-delay một lệnh của CPU.
- LWC2/SWC2 với kiểm tra alignment, cache-isolation store và quyềnCU2.
- Exception Coprocessor Unusable code11 với CE=2 khiCU2 tắt;
  COP1/COP3 không hiện diện cũng trả exception đúng loại thay vì Reserved Instruction.
- BC2F/BC2T với điều kiện COP2 luônfalse và delay slot thật.
- 32data +32control GTE registers; sign/zero extension, SXY push alias,
  IRGB/ORGB, LZCS/LZCR, FLAG write mask/summary.
- 22lệnh GTE thông dụng: RTPS, RTPT, NCLIP, MVMVA, SQR, OP, AVSZ3/4,
  GPF/GPL, DPCS, DPCT, INTPL, DCPL, NCS/NCT, NCCS/NCCT, NCDS/NCDT, CC, CDP.
- Số học fixed-point, MAC44-bit, saturation/flags, matrix/vector, XY/Z/RGB FIFO,
  phép chia projection dùng thuật toán UNR được mô tả trong PSX-SPX.
- Chụp lỗi CPU đầu tiên: PC, opcode, Cause/EPC/SR, delay-slot,32GPR,
  thanh ghi GTE và32opcode trước lỗi. Trace này không bị vòng lặp sau đó ghi đè.
- Log lưu cả opcode của Reserved Instruction gần nhất.
- Runner/UI/CLI dừng khi quan sát BIOS A0(40), thông báo nguyên nhân và cho Save log;
  không tiếp tục báo “đang chạy” mãi trong vòng fatal. CPU hardware exception vẫn
  được chuyển tới handler trước; không biến lệnh chưa hỗ trợ thànhNOP.
- UI hiện GTE commands và First CPU fault nếu có.

## Phạm vi chính xác

GTE là bộ xử lý chức năng mới, chưa được xác nhận cycle-accurate hay bit-exact
với toàn bộ test phần cứng. Chưa có pipeline busy/interlock, độ trễ ghi LZCS/IRGB
và đồng bộ IRQ của lệnh GTE. Một số cờ overflow trung gian, đặc biệt lựa chọnFC
bug của MVMVA, còn xấp xỉ. Opcode GTE không có trong danh sách hỗ trợ vẫn tạo
Reserved Instruction và lưu opcode; không tự bỏ qua nó.

CPU/GPU/timer vẫn dùng mô hình thời gian xấp xỉ. Các VSync timeout trong log cũ
chưa được kết luận đã sửa chỉ nhờ thêmGTE. CD-DA/XA, MDEC và memory card còn thiếu.
**Chưa có kiểm chứng GTA1 chạy được. Chưa có BIOS/game thật trong môi trường test.**
Không dùng bước nhảy bỏ qua A0(40), ép PC sang game hay file tiếng/logo giả để che lỗi.

## Thử lại

1. Giải nén vào thư mục mới, chọn Windows x64 Release, startup ps1_ui.exe, Ctrl+F5.
2. Tiêu đề phải là **PS1 Lab v18 - COP2 GTE and First Fault**.
3. Nạp BIOS -> Mount CUE GTA1 -> Boot Game CD -> Run.
4. Nếu dừng: Save log ngay. Nếu còn chạy nhưng không tiến: Pause -> Save log.
5. Log mới cần có `FIRST CPU FAULT` hoặc `last_reserved_opcode` nếu còn gặp lỗi CPU.
   Lần này không cần chờ thêm hàng trăm triệu lệnh để thu thập lỗi.

## Nguồn đối chiếu

- https://psx-spx.consoledev.net/geometrytransformationenginegte/
- https://psx-spx.consoledev.net/dev/ps1/cpu/cpuspecifications/
- https://psx-spx.consoledev.net/ps1/kernelbios/misc-functions/

Mã mới được viết từ tài liệu; không chép triển khai DuckStation. Không chứa BIOS
hoặc dữ liệu GTA1. Bài kiểm tra tạiPC0x8004b70c dùng opcode tự tạo, không phải
opcode lấy được từ BIOS của người dùng.

Kết quả kiểm thử đầy đủ: VALIDATION_V18.md (21bộ PASS và sanitizer cho GTE/COP2).
