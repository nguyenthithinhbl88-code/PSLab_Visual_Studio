# Bản18.1 — IRQ/FIFO và truy cập bus CD-ROM

## Log tại menu BIOS

Đĩa vẫn mount, BOOT `cdrom:\SLUS_001.06;1`,11track,304.282sector.
CPU không có Reserved Instruction, không vào SystemError; BIOS đang vẽ menu.
Drive chỉ thực thi2lệnh, lệnh cuối01h Getstat/Nop, chưa đọc sector nào.

Log cũ chưa có enable mask, response FIFO hay lịch sử lệnh nên chưa chứng minh
được toàn bộ nguyên nhân BIOS bỏ qua boot đĩa. Đây cũng không phải bằng chứng
file CUE bị hỏng hay GetID sai: BIOS chưa đi xa tới lệnh GetID trong log này.

## Những lỗi tìm thấy và sửa

1. IRQ enable của drive đang bằng0 sau reset. Đối chiếu reset của DuckStation,
   mask mặc định là1Fh. Bản này đặt1Fh khi tạo/reset controller; phần mềm vẫn có
   thể ghi0 để tắt IRQ. Nếu BIOS chờIRQ2 trước khi tự ghi mask, bản cũ không thể
   đưa thông báo hoàn tất đến CPU mặc dù phản hồi đã có.
2. ACK trước đây chỉ xóa flag, để lại phản hồi chưa đọc trongFIFO. Trong khi
   scheduler không tiến nếuFIFO còn dữ liệu, trạng thái này có thể chặn phản hồi
   thứ hai hoặc lệnh đã xếp hàng. Theo PSX-SPX, acknowledge drain resultFIFO;
   bản mới xóaFIFO khi acknowledge các bit ngắt.
3. BUSYSTS trước đây luôn0. Nay command đang gửi/chờ phản hồi hoặc đang queued
   hiệnbit7; bit này hạ khi phản hồi được đưa ra. Thời gian vẫn là xấp xỉ theo tick,
   không phải latency thực của firmware.
4. CPU đọc32bit ở1F801800h trước đây lần lượt đọc4port, vô tình lấy mấtRESULT/data.
   Bus8bit CD-ROM mặc định tắt auto-increment: word read phải lặp HSTS bốn lần.
   Bản mới xử lý halfword/word trên portCD và DEV5.bit13. Halfword ởRDDATA lấy
   hai byte liên tiếp từ cùngFIFO khi auto-increment tắt. DMA3 vẫn dùng đườngFIFO
   chuyên biệt; chưa mô phỏng toàn bộ bus-width/waitstate củaBIU.

## Chẩn đoán mới

Save log ghi index, enable, flag, busy, status, response/parameterFIFO,
queued responses/commands và DEV5. Ring64mục ghi submit, execute, response,
ACK, mask, sector cùng command/parameters/response bytes.
Không ghi mọi lần đọc trạng thái để polling không đẩy mất lịch sử lệnh.

## Kiểm thử

22bộ kiểm thử C++17 -O2 -Wall -Wextra -Werror đềuPASS.
cd_protocol_tests kiểm traIRQ reset, busy, ACK khi chưa đọc hếtRESULT,
queued command, explicit mask/unmask, IRQ edge và phản hồi2giai đoạn.
Có MIPS program thật gửiNop rồi đợiI_STAT.IRQ2, đọcRESULT vàACK thành công;
không tự ghiHINTMSK trước khi gửi command.
Kiểm tra word/halfword không ăn mấtRESULT khi auto-increment tắt, và đọc4port
khi bậtDEV5.bit13. Catch Stars từCUE tiếp tục chạy2triệu steps quaCLI.

## Thử GTA1 lại

Giải nén vào thư mục mới, Windows x64 Release, ps1_ui.exe, Ctrl+F5.
Tiêu đề **PS1 Lab v18.1 - CD IRQ and ACK Protocol**.
Nạp BIOS -> Mount CUE GTA1 -> Boot Game CD -> Run.
Nếu vẫn về menu: Pause -> Save log. Lịch sử mới sẽ cho thấy phản hồi từng lệnh,
thay vì chỉ tổng2command như log trước.

**Chưa xác nhận GTA1 chạy được; không có BIOS/đĩa thật trong môi trường phát triển.**
Windows UI chưa build/run tại đây. VSync timing, CD latency, XA/CD-DA, MDEC và
những phần còn thiếu từ bản18 vẫn chưa được coi là hoàn thiện.

## Nguồn

- PSX-SPX CD-ROM ports, HCLRCTL/ACK và BUSYSTS:
  https://psx-spx.consoledev.net/cdromdrive/
- PSX-SPX Memory Control, DEV5 auto-increment:
  https://psx-spx.consoledev.net/dev/ps1/system/memorycontrol/
- DuckStation CD-ROM reset/default IRQ mask (đối chiếu hành vi, không chép implementation):
  https://github.com/stenzek/duckstation/blob/master/src/core/cdrom.cpp

AddressSanitizer/UndefinedBehaviorSanitizer cũng PASS cho cd_protocol_tests,
cd_tests và disc_tests. Bài MIPS đợiIRQ2 được chạy lại sau thay đổi bus rộng.
