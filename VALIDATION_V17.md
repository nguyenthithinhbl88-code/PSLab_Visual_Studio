# Kiểm chứng bản 17 / 17.1

Bổ sung17.1:19 bộ test đều PASS sau thay đổi route; boot_route_tests PASS thêm
với ASan/UBSan. CLI Catch Stars CUE chạy2triệu steps thành công.
Chi tiết lỗi GTA1, giới hạn và đường thử: GTA1_BOOT_FIX.md.
Các số benchmark/WAV bên dưới là phép đo của bản17, không phải đo lại17.1.

Môi trường: Linux, g++ C++17, -O2 -Wall -Wextra -Werror -pthread.
Đã chạy18 bộ kiểm thử, tất cả PASS:

- core_tests, cpu_extended_tests (tệp cpu_extended.cpp)
- gpu_tests, polygon_tests, texture_tests, line_tests
- boot_tests, system_tests, platform_tests, dicr_tests
- spu_tests, timer_tests, cd_tests, video_tests, disc_tests
- spu_audio_tests, runner_tests, timer_sync_tests

spu_audio_tests kiểm tra các vector ADPCM đã biết, dấu/predictor, KON/KOFF/ENDX,
voice23, ADSR, mute, queue bound, sample divider, capture/DMA IRQ9, status requests,
volume/current-volume, sweep, reverb read-when-disabled/wrap/IIR và demo stereo.
runner_tests kiểm tra tiến triển worker, stop/join, restart có pacing, lỗi fetch,
truyền lỗi về UI và truy cập input/audio dưới khóa. Không phải bài chứng minh race-free.
timer_sync_tests có vòng MIPS thật chờ HBlank kết thúc trong giới hạn10.000 steps.

AddressSanitizer và UndefinedBehaviorSanitizer (-O1 -g) đều qua cho:
spu_audio_tests, runner_tests, timer_sync_tests, video_tests và disc_tests.
Không chạy ThreadSanitizer; chưa kiểm tra driver Windows.

ps1_audio_probe tạo examples/spu_test.wav bằng chính SPU của dự án:
44.100frame,2kênh,16-bit,1giây. Peak trái/phải3348; RMS khoảng2372/2373.
Đã xác nhận header/số mẫu/biên độ và hai kênh khác nhau bằng Python wave/struct.
Đây là hai tone thử gốc, không phải tiếng startup Sony.

CLI đọc catch_stars.cue và chạy2triệu steps với --wav thành công;
WAV có2604frame stereo, đúng floor(2.000.000/768).
Catch Stars không tạo voice nên WAV này im lặng là kết quả dự kiến.

## Đo hiệu năng có giới hạn

tools/core_benchmark.cpp chạy5triệu lệnh MIPS tự tạo, lặp đọc GPUSTAT, không có BIOS.
Cùng -O3 -DNDEBUG, không LTO,5 lượt xen kẽ trên máy chia sẻ:

| Core | Median MIPS | Khoảng MIPS |
|---|---:|---:|
| v16 |20,40|16,70–22,21|
| v17 |17,92|15,73–18,26|

Core mới có thêm xử lý SPU/timer nên bài này chậm hơn; không gọi đây là tăng tốc core.
Thay đổi giúp Run là bỏ trần100.000steps /16ms (~6,25Mstep/s trước cả chi phí vẽ),
ngân sách8ms và tách CPU khỏi UI. Chưa đo frontend Windows để gán mức tăng FPS.
Release tự bật IPO/LTO nếu compiler hỗ trợ; có thể chạy ps1_core_benchmark.exe
trên máy người dùng để đo local. Một lượt v17 với -O3 -flto đạt25,72MIPS;
đây là cấu hình khác, không dùng để kết luận tăng tốc so với bảng trên.
Bench không thay thế kiểm tra BIOS thực tế.

Chưa có WindowsSDK/Visual Studio để build/run ps1_ui trong môi trường này.
Chưa có BIOS thật/game thương mại trong phiên để kiểm tra boot/audio/compatibility.
Không có cơ sở cam kết60FPS hay đánh dấu một game retail bất kỳ là tương thích.
