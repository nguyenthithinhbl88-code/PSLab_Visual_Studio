# PSLab v29 — vá âm thanh và hiệu năng PS1

Bản này tập trung PS1/GTA1. Phần PS2 v28.2 được giữ lại. Không kèm BIOS/game thương mại.

## Âm thanh

Bản trước resample theo tốc độ phát sinh PCM của core. Khi core chỉ đạt 50% tốc độ, cách đó kéo dài tiếng nhưng đồng thời hạ cao độ. v29 thay đường phát bình thường bằng **WSOLA viết riêng**, thay đổi tempo bằng ghép các đoạn sóng tương đồng, giữ tần số/cao độ tốt hơn.

- Stereo dùng cùng vị trí ghép, giữ quan hệ pha trái/phải.
- Synthesis hop 512 frame, cửa sổ 1024, vùng tìm ±256 frame ở 44.1 kHz. Tìm thô rồi tinh để giảm CPU; ghép và tính toán ngoài khóa máy giả lập.
- Tempo hỗ trợ 0.25..2.0 theo tốc độ phát sinh PCM và mức đệm. Ước lượng lại mỗi 50 ms thay vì để trễ tới 250 ms sau startup.
- Ở đúng 1x, đường WSOLA giữ mẫu stereo nguyên vẹn trước fade/mute. Các tempo khác có thể có biến dạng transient/nhạc phức tạp; đây chưa phải chất lượng đã được nghe thử với GTA1.
- Khi hàng đợi sắp cạn, gửi một đoạn fade xuống 0 dài tối đa 128 frame, xây lại preroll rồi fade vào. Không thể khôi phục âm thanh chưa được core tạo ra; core chậm kéo dài vẫn có thể im lặng/rebuffer.
- Nếu input vượt 100 ms (4410 frame), cắt về 3072 frame, xóa overlap cũ và fade lại. Chừa khoảng trống giúp tránh vòng lặp overflow → reset → overflow. Counter dropped ghi số frame bị bỏ, không che giấu mất dữ liệu.
- Thiết bị WinMM được pause khi nạp các buffer đầu, rồi restart sau khi có dữ liệu, giảm nguy cơ bắt đầu với chỉ một block.
- Audio worker đăng ký **MMCSS Audio** khi Windows cho phép. Registration được thu hồi ngay trên cùng worker khi thoát; nếu đăng ký thất bại vẫn dùng lịch chạy thường. Link thêm Avrt.lib có trong Windows SDK.
- Emulated SPU/ADPCM/ADSR/reverb/CD-XA/CDDA, clock và dữ liệu PCM do SPU tạo không bị thay đổi bởi time stretching. Utility linear resampler vẫn có bài test nhưng không còn là đường phát bình thường.

WSOLA thêm lookahead và độ trễ. Cửa sổ 1024 + vùng tìm, cộng các buffer thiết bị, không phải một chế độ âm thanh không độ trễ. Mô hình tốc độ core vẫn xấp xỉ, chưa cycle accurate.

## Hiệu năng và độ đều khung hình

- RAM byte/halfword và scratchpad có đường truy cập trực tiếp, tránh đi qua các bộ giải mã DMA/MMIO trước. BIOS đọc byte/halfword cũng có đường nhanh, vẫn chỉ đọc. Giữ endian, mirroring, alias và lỗi alignment.
- Khi bảng debug ẩn, UI không còn format toàn bộ thanh ghi mỗi 250 ms.
- Khi bảng debug hiện, hoàn thành chuỗi text rồi **nhả mutex core trước SetWindowText/InvalidateRect**.
- Giữ lại vector pixel của UI để tái sử dụng dung lượng, tránh cấp phát lại buffer cho mỗi repaint.
- Khi đang chạy, repaint do VRAM đổi chỉ được yêu cầu sau khi bộ đếm video frame đã tiến; thay đổi cấu hình display vẫn cập nhật ngay. Giảm cập nhật ảnh trung gian trong cùng một frame model. Đây chưa phải snapshot chính xác tại VBlank, double buffering đầy đủ hay VSync phần cứng.
- Không bỏ lệnh GPU, không thay tốc độ CPU/timer, không giảm chất lượng rasterizer để đạt điểm benchmark.

## Kết quả đo cục bộ

Cùng compiler GCC C++17 -O2, cùng host, ba lượt mỗi bản. Chỉ là benchmark tổng hợp, không có BIOS/GTA1 thật trong môi trường này:

| Bài đo | Trước (trung vị) | Sau (trung vị) | Phạm vi kết luận |
|---|---:|---:|---|
| 8 triệu lượt byte/halfword RAM hỗn hợp | 289.904 ms | 88.691 ms | Đường truy cập RAM này nhanh hơn khoảng 3.27 lần |
| 5 triệu lệnh MIPS có LB/SB/LH/SH và nhánh | 135.991 ms | 119.375 ms | Khoảng 12.2% ít thời gian hơn trong workload này |
| 3 triệu lệnh vòng J/NOP | 102.403 ms | 100.605 ms | Gần như không đổi trong dao động đo |

Checksum/state cuối trùng nhau giữa trước/sau. Không suy ra GTA1 tăng 12% FPS hoặc chắc chắn đạt 60 FPS từ các phép đo này. Chưa có MSVC/Win32 runtime để đo hiệu năng/âm thanh Windows.

## Kiểm thử

- **43 bộ kiểm tra core PS1/PS2** qua GCC C++17 -O2 -Wall -Wextra -Werror.
- WSOLA: 15 tổ hợp gồm 110/440/1000 Hz ở tempo 0.5/0.75/1/1.5/2.0. Sai lệch tần số đo dưới ngưỡng 2.5%, kiểm tra năng lượng, tempo consumption và pha stereo.
- 1x: stereo noise mẫu tùy ý đi qua bit-for-bit trước gain.
- Atomic khi input thiếu, chunk lẻ, NaN, reset sau trim và fade tới im lặng.
- Mô phỏng 12 giây, nguồn thay tốc độ 0.5→1→0.75, giữ/thả burst 4 ms mỗi 200 ms: **0 underrun, 0 dropped**, 529408 frame output được nộp.
- Bộ audio mới chạy thêm ASan/UBSan, tắt LeakSanitizer do môi trường chạy.
- 200000 lượt byte/halfword RAM/alias, boundary scratch/BIOS, alignment, read-only và MMIO fallback.
- Kiểm tra hồi quy CPU, DMA/CD-ROM, GPU, SPU/XA và PS2 đều được giữ.

Việc chuẩn bị/restart WinMM, MMCSS, GUI mutex/repaint cần kiểm chứng trực tiếp trên Windows; chưa thể khẳng định cảm nhận âm thanh GTA1 trước khi nghe thực tế.

## Cách thử GTA1 trên Windows

1. Giải nén vào thư mục mới, Visual Studio → windows-release → build/startup **pslab**. Tiêu đề phải có **PSLab v29 - PS1 Audio and Performance**.
2. Load BIOS PS1 → Open disc chọn **CUE** của GTA1 nhiều track → Boot disc + Play theo flow hiện tại.
3. Giữ **Audio: On**, **Speed: 1x**, đóng bảng debug khi chơi. Có thể dùng Cinema mode để xem màn hình lớn.
4. Test cùng đoạn đường/lái xe trong khoảng 1 phút. Nghe nhạc, tiếng xe, hiệu ứng; thử Pause/Resume và Alt-Tab.
5. View → debug để xem `Tempo / WSOLA`, `Underruns`, `Dropped`, `MMCSS`, Video Hz model và MIPS. Nếu còn tiếng đứt, Export log sau đoạn thử để đối chiếu.

`MMCSS=0` không đồng nghĩa thiết bị audio lỗi; chỉ cho biết worker chưa đăng ký được ưu tiên multimedia. Video Hz model chưa phải phép đo FPS của riêng logic GTA1.

## Tài liệu tra cứu

Không sao chép core DuckStation hoặc mã SoundTouch vào dự án. WSOLA là hiện thực riêng theo nguyên tắc waveform-similarity overlap-add.

- DuckStation audio buffering/stretch/underrun reference: https://github.com/stenzek/duckstation/blob/master/src/util/core_audio_stream.cpp
- SoundTouch phân biệt tempo/pitch/playback rate và tradeoff sequence/seek/overlap: https://www.surina.net/soundtouch/README.html
- Nghiên cứu WSOLA gốc: https://www.isca-archive.org/eurospeech_1993/roelands93_eurospeech.html
- WinMM pause/restart: https://learn.microsoft.com/en-us/windows/win32/multimedia/stopping-pausing-and-restarting-playback
- MMCSS Audio: https://learn.microsoft.com/en-us/windows/win32/procthread/multimedia-class-scheduler-service
- Revert cùng thread: https://learn.microsoft.com/en-us/windows/win32/api/avrt/nf-avrt-avrevertmmthreadcharacteristics
