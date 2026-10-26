# Bản 17: âm thanh SPU và chạy BIOS trên worker

## Thay đổi

- CPU chạy trên worker riêng, không còn bị giới hạn 100.000 lệnh / WM_TIMER16ms
  hay ngân sách8ms mỗi lần gọi. Timer chỉ lấy input, bơm âm thanh và cập nhật UI.
- Speed:1x điều tiết theo steady_clock; Speed:Max chạy hết khả năng CPU.
  1x vẫn dựa trên mô hình cũ: một lần step xấp xỉ một clock, chưa là CPU cycle-accurate.
- Pause/reset/nạp file đều stop và join worker trước khi đổi trạng thái máy.
  UI lấy snapshot dưới mutex; GDI/waveOut không giữ khóa máy khi phát/vẽ.
- 24 voice ADPCM,5 predictor, pitch, bảng nội suy Gaussian4 điểm và lịch sử qua block.
- KON/KOFF, ENDX, loop start/repeat, ADSR, signed direct volume và volume sweep cơ bản.
- Mixer stereo44.100Hz, mute, noise và pitch modulation cơ bản.
- Reverb IIR/COMB/APF dùng RAM SPU, volume, ESA/wrap và master write-enable.
  Bộ lọc39-tap và thứ tự phần cứng được xấp xỉ bằng trung bình hai mẫu/giữ output;
  tiếng vang chưa bit-exact và cần đối chiếu bản ghi thật.
- DMA4 status request, IRQ9 khi đọc mẫu/DMA/capture/reverb; capture voice1/3,
  capture CD im lặng vì CD audio chưa có. Thanh ghi volume hiện tại có thể đọc.
- Timer0 nối dot clock/HBlank; Timer1 nối HBlank/VBlank; thêm các chế độ gate.
- Windows waveOut PCM stereo16-bit,6 buffer512frame, preroll3buffer (~35ms),
  reset/unprepare trước khi giải phóng. Mất thiết bị âm thanh không dừng CPU.
- Hàng đợi SPU giới hạn100ms; UI hiển thị Samples, Voices, Underruns, Dropped,
  MIPS, Video Hz; debug text cập nhật250ms; khung hình chỉ invalidate khi thay đổi.
- Preset Windows Release; bật IPO/LTO cho Release khi compiler hỗ trợ.
- Nút SPU test và WAV test gốc, không dùng BIOS để kiểm tra thiết bị âm thanh.
- CLI --wav ghi PCM do chính SPU tạo. Save log có SPU/IRQ và số Reserved exceptions.

## Kiểm tra BIOS thật trên Windows

1. Giải nén vào thư mục mới, không ghi lẫn vào bản cũ.
2. Visual Studio2022 -> Open Folder -> ps1-lab -> chọn windows-release.
   Nếu không thấy presets: dùng lệnh CMake trong README và --config Release.
3. Chọn Startup Item ps1_ui.exe, Ctrl+F5. Kiểm tra tiêu đề v17.1.
4. SPU test: nghe hai cao độ ở hai kênh trái/phải; Pause để dừng.
5. Nap BIOS (512KiB) -> Speed:1x -> Sound:On -> Run. Lần này chưa mount CD.
6. Chờ20giây thực, xem logo, chuyển động, âm thanh và Video Hz/MIPS.
7. Nếu chậm, mất tiếng, đứng hoặc nhiều Underruns: Pause -> Save log.
   Gửi log cùng ảnh chỉ số; không cần gửi BIOS. Samples tăng mà Voices=0 có thể
   nghĩa là BIOS chưa bật voice hoặc đã phát xong, không chứng minh loa hỏng.

Không chèn file âm thanh startup để che lỗi SPU. Tiếng BIOS phải đến từ mẫu/thanh ghi
mà chính BIOS gửi cho SPU. Bản mới chưa được chạy với BIOS thật hay thiết bị Windows
trong môi trường phát triển hiện tại; chưa xác nhận hết tiếng startup hoặc menu BIOS.
Không cam kết60FPS. Video Hz là số field theo mô hình, không phải số ảnh mới vẽ.
Speed:Max có thể làm rơi audio; dùng1x khi đánh giá tiếng.

## Thử game 2D thương mại

Đường thử: Nap BIOS -> Mount CD (chọn CUE gốc, giữ BIN cạnh nó) -> Boot Game CD -> Run.
Không dùng Homebrew EXE để kết luận tương thích game retail vì nó bỏ khởi tạo kernel.

**Chưa có game thương mại nào được xác nhận chạy trong bản17.**
Không có file game/BIOS trong môi trường kiểm thử; thêm SPU không đủ để bảo đảm boot game.
Cần tên game, vùng/mã đĩa và Save log sau khi thử để xác định lỗi thực tế.

| Phần | Trạng thái |
|---|---|
| GPU textured sprites/polygons/lines | Subset phần mềm từ bản16, có kiểm thử hồi quy |
| BIN/CUE/ISO, sector read, DMA3, pad | Đã có; timing/drive command chưa đầy đủ |
| SPU ADPCM/stereo/reverb | Mới, cần kiểm chứng bằng BIOS/game thật |
| GTE/COP2 | Chưa triển khai; có thể gây vòng exception |
| MDEC / phim intro | Chưa triển khai |
| CD-XA / CD-DA | Chưa giải mã/phát; nhạc từ CD sẽ thiếu |
| Memory card / savestate | Chưa có |
| CPU/GPU/CD/SPU timing | Xấp xỉ, chưa cycle-accurate |

## Ghi WAV phục vụ chẩn đoán

Developer PowerShell, sau khi build Release:

```powershell
.\build-vs\Release\ps1_audio_probe.exe spu_test.wav
.\build-vs\Release\ps1_lab.exe --bios "C:\PS1\bios.bin" --steps 160000000 --wav startup.wav > bios_audio_log.txt
```

WAV44.1kHz là âm thanh theo thời gian giả lập, có thể nghe liên tục dù CPU chạy chậm
hơn thời gian thực. Điều đó không chứng minh frontend đã full-speed.
CLI bị giới hạn200triệu steps; stdout chỉ chứa diagnostic, WAV được ghi riêng.

Chi tiết nguồn: RESEARCH.md. Kết quả kiểm thử: VALIDATION_V17.md.
