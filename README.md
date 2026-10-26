# PSLab — bản 30.1

C++17 / Win32 / Visual Studio 2022. PS2 thêm **EE hardware/MMIO, timer và ngắt, normal GIF DMA, CPU branch-likely và GIF PACKED**. Demo mới: Tools → PS2 EE laboratory → **DMA GPU demo → Run continuously**.

Hotfix 30.1 sửa Wired tại BFC00854, thêm COP0/TLB register bank; chưa có MMU translation. Đọc **PS2_WIRED_FIX_V301.md**.

Đọc **PS2_HARDWARE_V30.md** để xem phạm vi và giới hạn. **Chưa xác nhận boot BIOS PS2 hoàn chỉnh hay chạy game PS2 thương mại.**

PS1 giữ audio WSOLA và tối ưu v29; xem **PS1_AUDIO_PERFORMANCE_V29.md**.
Windows: build/startup target **pslab**, chạy pslab.exe, tiêu đề **PSLab v30.1 - PS2 Hardware and GIF DMA**.

# PS1 Lab — bản 24

Dự án giả lập PS1 C++17/Win32 cho Visual Studio 2022. Bản15 đã được người dùng
xác nhận hiện màn logo Sony Computer Entertainment bằng BIOS thật.
Bản16 thêm CD ISO/BIN/CUE và game homebrew Catch Stars có điều khiển.
Bản17 thêm SPU ADPCM/ADSR/Gaussian/reverb, đầu ra stereo Windows và CPU worker.
Bản21.1 bổ sung DMA burst chopping cho GPU/CD/SPU; xem **DMA_CHOPPING_FIX_V211.md**.
Bản21 mở rộng DMA/MMIO, thêm CD-DA, XA-ADPCM và phát âm thanh thích ứng tốc độ.
Đọc **COMPAT_AUDIO_UPDATE_V21.md** để xem thay đổi, kiểm thử và giới hạn.
Bản20.1 sửa đọc byte/halfword trên DMA0..6, DPCR và DICR; xem **DMA_SUBWORD_FIX_V201.md**.
Bản20.0 bổ sung MDEC chức năng và DMA0/DMA1 để xử lý reset, bảng giải mã và dữ liệu hình ảnh; xem **MDEC_UPDATE_V20.md**.
Bản19.1 thêm khoảng cách tối thiểu giữa ACK ngắt CD và phản hồi tiếp theo, sửa lỗi Init bị lặp; xem **CD_INIT_IRQ_FIX_V191.md**.
Bản19.0 sửa load pipeline CPU, SB/SH tới DMA và tách bộ đệm sector khỏi FIFO CD; xem **CPU_CD_UPDATE_V19.md**.
Bản18.1 sửa giao thức CD IRQ/ACK/bus; đọc **CD_BIOS_PROTOCOL_UPDATE.md**.
Bản18 thêm COP2/GTE chức năng và chụp lỗi CPU đầu tiên; đọc **GTE_BIOS_UPDATE.md**.
**Ảnh người dùng xác nhận bản21.1 đã vào menu và màn chơi GTA1. GTE/MDEC/timing chưa chính xác hoàn toàn; CD-XA/CD-DA đã có đường phát chức năng nhưng chưa kiểm chứng với GTA.**

Bản17.1 làm rõ đường boot và chẩn đoán lỗi GTA1; đọc **GTA1_BOOT_FIX.md**.

Đọc **BIOS_AUDIO_UPDATE.md** để thử BIOS mượt/âm thanh và xem giới hạn.

## Mở trong Visual Studio
Giải nén vào thư mục mới. Cài workload Desktop development with C++ và
C++ CMake tools for Windows -> File / Open / Folder -> chọn ps1-lab ->
windows-release -> Startup Item pslab.exe -> Ctrl+F5.
Tiêu đề phải là **PSLab v30.1 - PS2 Hardware and GIF DMA**.

Developer PowerShell:
```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release
ctest --test-dir build-vs -C Release --output-on-failure
.\build-vs\Release\pslab.exe
```

## Thử âm thanh và BIOS trước
SPU test -> nghe hai kênh -> Pause. Sau đó Nap BIOS -> Speed:1x -> Sound:On -> Run.
Không cần bấm Run10k liên tục. Nếu chậm/đứng: Pause -> Save log.
Bản21.1 chưa được kiểm tra với BIOS thật trên Windows trong môi trường này.

## Chơi thử game CD đi kèm, không cần BIOS
1. Mount CD -> chọn **examples/catch_stars.cue**. Giữ .cue và .bin cùng thư mục.
2. Bấm **Homebrew EXE -> Run**.
3. Dùng **trái/phải** điều khiển thanh xanh, hứng vật vàng rơi xuống. Thanh trên
   trái là điểm, thanh đỏ trên phải là lượt còn lại; trượt3 lần sẽ reset điểm/lượt.
4. Bấm **Phong hinh** để xem lớn, Esc hoặc Pause để dừng.

Game là chương trình MIPS gốc của dự án, lấy EXE qua SYSTEM.CNF trong ISO9660,
sau đó tự gửi lệnh CD, đọc sector qua DMA3, vẽ bằng GP0 và đọc digital pad qua SIO0.
Không có byte BIOS/game/asset Sony. Đọc **GAME_GUIDE_VI.md**.

## Thử đĩa của bạn qua BIOS
**Nap BIOS -> Mount CD -> chọn .cue -> Boot Game CD -> Run.**
Nap BIOS cần file512KiB, reset máy và giữ đĩa đã mount. Có thể nạp BIOS trước hoặc sau Mount CD.
Boot Game CD giữ BIOS/đĩa, reset RAM/CPU/GPU/drive rồi chạy từ reset vector.
Nếu bộ đĩa nhiều BIN, chọn CUE để giữ track/pregap đúng; không đổi đuôi file thành ISO.
Region CD: Auto theo prefix BOOT hoặc chọn USA/Europe/Japan/Unlicensed cho GetID
emulated. GetID không phải xác thực wobble từ đĩa gốc.

Homebrew EXE nạp trực tiếp theo SYSTEM.CNF hoặc PSX.EXE;1, **không khởi tạo BIOS
kernel/services**; đường này dành cho homebrew tự khởi tạo, không thay thế boot retail.
Khi dừng/đen: Pause -> Save log; log có track layout, drive command/mode/error,
FIFO, GP0, video và input. Không cần gửi BIOS.

## Thay đổi chính
- ISO2048, BIN raw Mode1/Mode2 2352; CUE BINARY nhiều file/track, INDEX00/01,
  PREGAP; Mode2/2336 và cooked track2048.
- Sector data2048/raw2340, TOC/location/seek/ReadN/ReadS, IRQ2 và command queue.
- DMA3 chờ dữ liệu và truyền nhiều sector theo từng phần; không crash vì chưa có DRQ.
- ISO9660 đọc thư mục con, SYSTEM.CNF BOOT, path không phân biệt hoa/thường.
- Digital pad bàn phím; GPU flat/Gouraud line/polyline; giữ texture/sprite của bản14.
- CPU đọc/ghi word RAM và fetch BIOS qua đường nhanh; giữ MMIO và alignment checks.
- UI Video Hz(model)/CPU MIPS đo theo đồng hồ thật, cập nhật mỗi giây khi Run.
  Video Hz là tốc độ field của model hiện tại, không phải FPS game hoặc FPS paint.

Đọc **DISC_UPDATE.md** để xem phạm vi hỗ trợ, test và tài liệu tham khảo.
Timing CPU/video/CD vẫn gần đúng; CD-DA/XA có giải mã/phát chức năng, chưa bit-exact;
chưa có memory card/save state, CHD/PBP/ECM/subchannel hay tương thích retail đầy đủ.

## Điều khiển pad
Mũi tên=D-pad, Z=Cross, X=Circle, A=Square, S=Triangle,
Q/W=L1/R1, E/R=L2/R2, Enter=Start, Shift=Select.
Chỉ đọc phím khi cửa sổ emulator ở foreground. Mất focus sẽ nhả nút.

## Demo và kiểm thử
GPU demo / Polygon / Texture demo dùng chương trình MIPS tự viết.
Open EXE nạp PS-X EXE, không phải EXE Windows.
examples/homebrew.iso và dma_polygon.exe là fixture cũ; boot_test_rom.bin là
ROM chẩn đoán tự viết, không phải BIOS Sony.

35 bộ kiểm thử lõi PASS trên GCC C++17 với warnings-as-errors. GTE/COP2 và stress44.000lệnh cũng PASS với ASan/UBSan; xem VALIDATION_V18.md.
Bốn bộ mới của bản21 PASS với ASan/UBSan (leak detection tắt do giới hạn ptrace).
Host Linux chưa build/chạy UI Windows. Cần xác nhận Visual Studio trên máy bạn.
Các file *_UPDATE cũ ghi trạng thái lịch sử; README và DMA_CHOPPING_FIX_V211.md mô tả bản21.1; COMPAT_AUDIO_UPDATE_V21.md ghi bản21; DMA_SUBWORD_FIX_V201.md ghi bản20.1; MDEC_UPDATE_V20.md mô tả phần giữ từ bản20.0; CD_INIT_IRQ_FIX_V191.md mô tả phần giữ từ bản19.1; CPU_CD_UPDATE_V19.md mô tả phần giữ từ bản19.0; GTE_BIOS_UPDATE.md ghi phần CPU từ bản18; BIOS_AUDIO_UPDATE.md mô tả phần âm thanh giữ từ bản17. DISC_UPDATE.md mô tả phần CD được giữ lại từ bản16.

Linux CLI:
```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/ps1_lab --disc examples/catch_stars.cue --steps 2000000
./build/ps1_lab --bios bios.bin --disc your_disc.cue --steps 20000000
```

Tái tạo game/đĩa gốc: `python tools/make_cd_game.py`.
BIN demo có vùng EDC/ECC zero để test emulator, không dùng làm đĩa burn/kiểm chuẩn ECC.
MIT cho mã dự án tự viết; không sao chép implementation của giả lập khác.

Bản22: đọc AUDIO_PERFORMANCE_V22.md. Luồng phát âm thanh riêng theo event WinMM, fade/mute, giảm thời gian giữ mutex và tối ưu fetch/timer/MDEC. Chưa đo lại GTA hoặc nghe âm thanh trên Windows tại môi trường này.

Bản23: xem KEYBOARD_PAD_V23.md. Bấm vùng game, mũi tên di chuyển, Z/X/A/S là Cross/Circle/Square/Triangle; Enter Start; Shift Select; Q/W L1/R1; E/R L2/R2. Nút Keyboard Pad mở hướng dẫn. Alt-Tab tự nhả phím.

Bản24 xây lại giao diện và thêm ảnh màn chào tạo riêng; xem RETRO_STUDIO_V24.md. Debugger ở View → Show debugger, demo ở Tools. CMake sao chép assets cạnh EXE; giữ thư mục này khi chuyển ứng dụng. UI chưa được build/chạy bằng Windows SDK tại môi trường này.

Bản26 mở rộng HI/LO/COP0 và19 thanh ghi GS. Xem EE_GS_REGISTERS_V26.md. Tools → PS2 EE laboratory → CPU + GS demo → Run1k; RAM[200]=23(hex35), BGCOLOR=bb55. Chưa có renderer hay bootBIOS PS2.
