# Bản 21 — DMA/MMIO, CD-DA, XA và phát âm thanh khi chạy chậm

## Lỗi trong log ngày 26/10/2026

Log `ps1_bios_log(20261026-053240).txt` dừng sau 412012681 lệnh:

- Lệnh gây lỗi: `80127f4c: a0640002` (`SB a0,2(v1)`).
- `v1=1f8010f4`, `a0=00000082`; địa chỉ ghi là `1f8010f6`.
- DICR trước ghi: `008a0000`. Trước đó `LBU` đã đọc đúng byte `8a`.
- Ghi đúng theo bus DMA: `00000082 << 16 = 00820000`.

Bản 20.1 xử lý phía đọc nhưng bỏ sót phía ghi lệch byte. Test hồi quy dùng nguyên
chuỗi `90640002 / 00021027 / 00822024 / a0640002`: chạy mã trước vá tái hiện đúng
`Unsupported/read-only bus write @ 0x1f8010f6`; mã mới đi qua và DICR thành `00820000`.
Không giả lập bằng cách bỏ qua lệnh hoặc cho CPU nhảy qua đoạn game.

## Phạm vi thanh ghi bổ sung/sửa

| Vùng | Hành vi bản 21 |
|---|---|
| DMA0..6 MADR, `1f801080 + n*10h` | SB/SH mọi lane hợp lệ, GPR đầy đủ dịch theo địa chỉ, mask 24 bit |
| DMA0..6 BCR, `1f801084 + n*10h` | Byte/halfword có byte enable, giữ lane khác |
| DMA0..6 CHCR, `1f801088 + n*10h` | SB/SH theo bus 32 bit |
| Mirror CHCR, `1f80108c + n*10h` | Đọc/ghi cùng CHCR thật, cả word và subword |
| DPCR/DICR, `1f8010f0..f7` | Ghi dịch fullword; giữ quy tắc write-one-to-clear và tính IRQ DICR |
| DMA `1f8010f8..ff` | Bỏ qua ghi theo vùng không sử dụng được tài liệu ghi nhận |
| SPU `1f801c00..1f801e5f` | Đọc byte; SB chẵn ghi low16 GPR, SB lẻ bị bỏ qua |
| SPU `1f801e60..7f` | Đọc 0/ghi bỏ qua cho vùng chưa sử dụng; không vượt mảng volume |
| SPU `1f801e80..1f801fff` | Ghi bỏ qua ở vùng không sử dụng |
| GPUSTAT `1f801814..17` | Đọc byte/halfword đúng lane |
| MDEC status `1f801824..27` | Đọc byte/halfword đúng lane |

Giữ KUSEG/KSEG0/KSEG1 aliases, alignment checks và lỗi ở MMIO chưa hỗ trợ.
BCR khác với các thanh ghi DMA còn lại: không được áp dụng một phép read-modify-write
chung cho DICR vì có thể vô tình xóa cờ ngắt.

## Âm thanh CD

- `Play 03h` với track BCD hoặc vị trí hiện tại; đọc PCM stereo signed16 little-endian
  từ track AUDIO/2352 trong CUE. Track data khi Play tạo im lặng.
- 588 frame/sector ở 44.1 kHz; trạng thái Play, Pause/Stop/Init/Seek, end-disc INT4,
  auto-pause khi chuyển track, báo cáo vị trí INT1 cơ bản và GetlocP.
- ATV0..3 staged/apply bằng ADPCTL bit5; stereo, mono, đổi kênh và clipping.
- Mute/Demute drive và ADPMUTE cho XA tách riêng; ADPMUTE không làm câm CD-DA.
- XA-ADPCM 4/8-bit, mono/stereo, 37800/18900 Hz, bốn predictor, shift/saturation,
  hàng đợi giới hạn và bộ đếm lỗi coding. Nội suy về 44.1 kHz bằng linear.
- ReadN/ReadS chọn Mode2 audio+realtime theo Setmode bit6 và Setfilter file/channel.
  Sector XA đã giải mã không xuất thành dữ liệu CPU/INT1; data xen kẽ vẫn đi qua DMA3.
- CD input vào capture RAM trái/phải trước gain; SPU CD volume, CD input enable,
  CD reverb send và main volume. CD input không phụ thuộc voice mute/enable.

## Phát âm thanh trên Windows

Log có 62 underrun, VideoHz khoảng 28.95 và 16.53 MIPS. Model chưa đạt tốc độ thật
trên lần chạy này. Đầu ra cũ luôn tiêu thụ 44.1k frame/giây dù SPU sản xuất chậm hơn.
Bản mới đo tốc độ sản xuất PCM và điều chỉnh lượng mẫu tiêu thụ bằng linear resampling,
phản hồi theo độ đầy hàng đợi, giữ stereo, preroll và hàng đợi giới hạn. Worker yield
sau mỗi batch để UI/audio có cơ hội lấy mutex. Log/UI thêm `audio_ratio`.

Đây là phát chậm theo tốc độ giả lập: khi ratio dưới 1, âm thanh có cao độ thấp hơn.
Không phải time-stretch giữ nguyên cao độ và không biến CPU chậm thành full speed.
Chưa nghe/kiểm tra waveOut trên Windows tại môi trường này.

## Kiểm chứng

31 bộ kiểm thử lõi C++17 PASS với `-O2 -Wall -Wextra -Werror`.
Cả bốn bộ mới PASS với AddressSanitizer/UndefinedBehaviorSanitizer. LeakSanitizer
phải tắt vì môi trường chạy dưới ptrace; chưa xác nhận leak checking.

Bốn bộ mới:

- `mmio_compat_tests`: chuỗi GTA gây lỗi, DMA lanes/masks/W1C/mirrors/aliases,
  SPU byte bus và status reads.
- `cdda_tests`: CUE PCM tự tạo, seek track, 588 frame, mono/stereo/cross routing,
  Mute/Demute, apply, auto-pause/end-disc INT4 và CD→SPU capture/gain.
- `xa_audio_tests`: dấu/nibble/byte, predictor golden, sample counts/rates, mono,
  queue bounds, reserved coding, filter và dữ liệu xen kẽ.
- `audio_resampler_tests`: stereo interpolation, mute, underrun atomic và 1000 block
  liên tục từ nguồn chạy nửa tốc độ, không trôi hàng đợi.

Các fixture đều tự tạo, không chứa BIOS, GTA hay nội dung Sony.

## Giới hạn cần biết

Chưa có BIOS/game thật ở môi trường kiểm thử; không xác nhận vào menu GTA 1 hay gameplay.
Log người dùng xác nhận BIOS đã nạp và thực thi EXE GTA, nhưng dừng trong phần đọc CD.
Chưa có FMV thương mại nào được kiểm chứng. CPU/video/DMA timing còn gần đúng;
MDEC/GTE chưa bit-exact. XA chưa có zigzag 25-point, emphasis, tự chọn channel đầu tiên
hay sound-map upload từ RAM. CD-DA chưa có fast-forward/backward, double-speed playback
và peak report chính xác; report đang trả peak0. Cơ học seek/pause và boundary pregap
chưa cycle accurate. Không tuyên bố tương thích thương mại hoàn chỉnh.

## Thử bản mới

Giải nén thư mục mới, mở bằng Visual Studio, build x64 Release. Tiêu đề:
**PS1 Lab v21 - DMA Compatibility and CD Audio**.

`Nap BIOS → Mount CD (chọn .cue) → Boot Game CD → Run`.

Save log mới có thêm CD audio playing/track/frame/sector/ATV, XA decoded/queued/dropped/
invalid coding và playback ratio để phân biệt lỗi giải mã với thiếu tốc độ máy chủ.
