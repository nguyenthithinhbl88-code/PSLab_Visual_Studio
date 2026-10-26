# v17.1 — chẩn đoán đường boot GTA 1

## Log người dùng cho thấy gì?

Đĩa có BOOT `cdrom:\SLUS_001.06;1`,11 track, track1 MODE2/2352 và10 track AUDIO.
CPU dừng sau17.748 lệnh; CD commands/sectors_read=0, GPU words=0, SPU CNT=0.
Trace gần nhất chạy qua các opcode0 trong RAM thấp rồi tới các từ dạng con trỏ
ở0x1000c/0x10014/0x1001c. Ví dụ0x8010a84c khi bị đọc như opcode là
`lb s0,-0x57b4(zero)`, dẫn tới địa chỉ0xffffa84c. Đây là giải mã đúng của từ đó;
không có cơ sở vá sign-extension của CPU hay ánh xạ địa chỉ lỗi về RAM.

R9=0x2b,R10=0xa0 và các tham số R4/R5/R6 phù hợp với lời gọi
A(2Bh) memset(dst,fillbyte,len). Tài liệu PSX-SPX quy định A0/B0/C0 là vector ở RAM,
do BIOS khởi tạo. Loader trực tiếp của bản17 tạo RAM trắng và không khởi tạo kernel.

**Suy luận có bằng chứng:** trạng thái rất phù hợp với EXE gọi BIOS khi RAM vector
chưa có handler, sau đó chạy qua RAM0 và dữ liệu của game. Log cũ không giữ được
trace từ điểm gọiA0 tới lỗi nên chưa phải chứng minh toàn bộ luồng bằng game thật.
Đây chưa là bằng chứng renderer/SPU hỏng hay game đã vào gameplay.

Nguồn:
- https://psx-spx.consoledev.net/dev/ps1/kernelbios/function-summary/
- https://psx-spx.consoledev.net/dev/ps1/kernelbios/memory-map/
- https://psx-spx.consoledev.net/ps1/kernelbios/memory-fill-copy-compare-slow/

## Đã sửa

- Đổi nút Boot EXE CD thành **Homebrew EXE**, nhắc rõ không khởi tạo kernel.
- Đổi nút Boot BIOS CD thành **Boot Game CD** để chỉ rõ đường thử game retail.
- Nạp BIOS giữ đĩa CUE đang mount và region; reset CPU/RAM/GPU/SPU/controller.
  Nạp file sai kích thước/đọc lỗi không làm mất máy hoặc đĩa hiện tại.
- Gắn nhãn direct-exe sau load_exe; khi direct EXE vàoA0/B0/C0 và cả16byte vector
  đều0, dừng ngay với thông báo thiếu kernel và hướng dẫn boot đúng.
  Đây là kiểm tra chẩn đoán loader; không giả lập HLE hay thực thi hộ hàm memset.
- Handler RAM do homebrew tự cài vẫn thực thi bình thường; đường BIOS/bareCPU
  không bị kiểm tra này chặn. Boot BIOS CD xóa nhãn direct.
- Log ghi boot mode, BIOS reset word, các vectorA0/B0/C0, lời gọi BIOS gần nhất,
  số hàm, địa chỉ trở về và bốn tham số. Không còn phải chỉ dựa vào32opcode cuối.
- Vẫn giữ lỗi bus nghiêm ngặt tại địa chỉ không được hỗ trợ.

## Thử lại GTA 1

1. Giải nén bản17.1 vào thư mục mới, chọn **windows-release**, **ps1_ui.exe**, Ctrl+F5.
2. **Nạp BIOS -> Mount CD -> chọn CUE GTA1 -> Boot Game CD -> Run.**
   Nếu đã Mount CD trước, nạp BIOS giờ sẽ giữ lại đĩa.
3. Speed:1x, Sound:On. Không dùng Homebrew EXE hoặc Open EXE cho lần thử này.
4. Nếu đứng/đen/lỗi khác: Pause -> Save log và gửi log mới.

Bản sửa này sửa/chẩn đoán việc boot sai đường, **chưa xác nhận GTA1 chạy được**.
Đĩa có10 audio track nhưng CD-DA vẫn chưa phát. GTE/MDEC và timing còn thiếu từ bản17.
Không có BIOS/game thật trong môi trường phát triển để kiểm tra end-to-end.
Windows UI chưa build/run tại đây.

## Kiểm thử

boot_route_tests dùng EXE MIPS tự tạo để gọi A0(2Bh) từ RAM trắng và xác nhận dừng
ngay tại vector, không chạy17.000NOP; kiểm tra handler hợp lệ vẫn được chạy.
CUE của homebrew gốc dùng để kiểm tra giữ file handle/media/region khi nạp BIOS,
kiểm tra file BIOS sai không thay đổi máy và BIOS restart xóa direct mode.
Không đưa ROM hay game Sony/Take-Two vào bộ kiểm thử.

Kết quả:19 bộ kiểm thử C++17 -O2 -Wall -Wextra -Werror đều PASS.
boot_route_tests cũng PASS với AddressSanitizer/UndefinedBehaviorSanitizer.
CLI chạy lại Catch Stars CUE2triệu steps thành công và có diagnostic mới.
