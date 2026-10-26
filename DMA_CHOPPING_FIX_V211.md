# v21.1 — DMA chia đợt (chopping)

Log `ps1_bios_log(20261026-055425).txt` xác nhận bản21 vượt qua ghi DICR và
DMA3 đã chép 8 word đầu sector LBA3869 vào RAM14c7a8. Lỗi tiếp theo:

- `80127fe4: acb50000` = SW s5,0(a1).
- a1=1f8010b8, s5=11400100, MADR=14cba8, BCR=1f8 (504 word).
- CHCR bit8 bật chopping, mode0, DMA window 1 word, CPU window16 tick.
- Bản21 chủ động dừng với `DMA chopping unsupported` trước khi đọc 504 word còn lại.

Bản21.1 thêm burst chopping chức năng cho DMA2/GPU, DMA3/CD và DMA4/SPU:

- Chia truyền theo CHCR bits16..18, nghỉ CPU theo bits20..22.
- Chỉ tick phần cứng mới giảm thời gian nghỉ; đọc/ghi thanh ghi và DPCR không làm
  thời gian trôi thêm. Tiếp tục chạy sau khi trigger bit28 đã xóa ở đợt đầu.
- Giữ busy bit24 tới đợt cuối, cập nhật MADR và low16 BCR theo dữ liệu đã truyền;
  BCR=0 ban đầu tương ứng65536 word.
- CD thiếu dữ liệu hoặc DPCR tắt thì chờ, giữ tiến độ. Không lặp/mất word khi tiếp tục.
- Ngắt DICR/IRQ3 chỉ ở hoàn tất toàn bộ; vẫn tôn trọng enable/mask của game.
- Ghi CHCR dừng/khởi động lại xóa lịch nghỉ và tiến độ cũ.
- Save log thêm channel/control/address/block/remaining/cpu_wait/bursts.

Kiểm thử `dma_chopping_tests` phát lại đúng opcode và CHCR từ log, xác minh toàn bộ
504 word, ranh giới thời gian nghỉ, FIFO ngắn, DPCR suspend/resume, BCR/MADR,
trigger/busy, enable/IRQ, cancel, GPU/SPU. Fixture dữ liệu tự tạo, không chứa game.
Mã DMA trước vá được chạy lại với test để xác minh tái hiện lỗi cũ.
32 bộ kiểm thử lõi PASS trên GCC C++17 với warnings-as-errors. Test chopping mới
PASS với ASan/UBSan; LeakSanitizer tắt do giới hạn ptrace của môi trường.

Giới hạn: một tick vẫn gần bằng một lệnh CPU. Chưa mô hình stall CPU trong cửa sổ
DMA, thời gian word theo waitstate, arbitration/priority và overhead phần cứng.
Đây là chopping chức năng, chưa cycle accurate. Chopping MDEC và tác dụng bit8 ở
sync modes1/2 chưa hỗ trợ; vẫn báo lỗi rõ ràng. Chưa kiểm chứng vào menu GTA1.
Âm thanh CD-DA/XA và phát thích ứng từ bản21 được giữ lại; log mới vẫn có underrun
và tốc độ model khoảng nửa thời gian thật, không xác nhận âm thanh đã mượt.

Tham khảo chính: https://psx-spx.consoledev.net/dmachannels/ và
https://psx-spx.consoledev.net/dev/ps1/system/dmachannels/ (đọc trực tuyến26/10/2026).

Build mới có tiêu đề **PS1 Lab v21.1 - DMA Chopping**.
Giải nén thư mục mới → build Release → Nạp BIOS → Mount CUE → Boot Game CD → Run.
Không có BIOS/đĩa thương mại trong môi trường test; UI Windows chưa được build tại đây.
