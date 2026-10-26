# Kiểm chứng bản18

Môi trường Linux/g++ C++17; -O2 -Wall -Wextra -Werror -pthread.
**21 bộ kiểm thử đều PASS** sau thay đổi core/COP2:

boot_route_tests, boot_tests, cd_tests, cop2_tests, core_tests, cpu_extended_tests,
dicr_tests, disc_tests, gpu_tests, gte_tests, line_tests, platform_tests, polygon_tests,
runner_tests, spu_audio_tests, spu_tests, system_tests, texture_tests, timer_sync_tests,
timer_tests và video_tests.

Hai bộ mới được chạy lại sau khi thêm các trường hợp biên:

- gte_tests: kiểm tra vector số học có kết quả tính trước, các kiểu thanh ghi,
  FLAG, FIFO, signed LZCS, matrix/translation, clipping, average depth,
  phép chiếu UNR, color interpolation, lighting; toàn bộ22opcode đã triển khai.
  Có kiểm tra overflow MAC44-bit ở cả hai biên và tích lũy cờ trongRTPT.
- cop2_tests: thực thi opcode MIPS thật, GPR load delay, LWC2/SWC2,
  quyềnCU2/CE, alignment, commands, BC2F delay slot; snapshot lỗi trong delay slot,
  giữ opcode sau2.000lệnh, và worker tự dừng ở A0(40) sau lỗi CPU trong dưới10lệnh.
  PC0x8004b70c trong bài test là địa chỉ từ log, opcode tại đó do dự án tự tạo;
  không phải bản tái hiện bằng byte BIOS của người dùng.

**AddressSanitizer/UndefinedBehaviorSanitizer (-O1 -g) PASS** cho gte_tests,
cop2_tests và ps1_gte_stress.

ps1_gte_stress dùng seed0x50533118,2.000bộ thanh ghi ngẫu nhiên và22lệnh/bộ
(44.000lệnh); kiểm tra phạm vi OTZ/SZ, bit reserved và summary của FLAG.
Stress test nhằm phát hiện lỗi bộ nhớ/số học và invariant, không chứng minh
GTE bit-exact hay tương thích game.

CLI chạy lại Catch Stars từ CUE trong2triệu steps, không Reserved Instruction,
có diagnostic mới. Game homebrew giữ pipeline CD/DMA3/GPU/pad đã kiểm tra.

Chưa có BIOS/game GTA1 để thử boot end-to-end tại đây.
Chưa build/run Windows UI bằng WindowsSDK/Visual Studio trong môi trường này.
Không có phép đo mới chứng minh tăng FPS, startup mượt hay game thương mại chạy được.
Số đo benchmark bản17 là kết quả lịch sử, không áp cho bộ GTE mới.
