# PSLab — bản 38: CACHE IXIN khi BIOS khởi tạo

Log v37 đã in **32 MB RAM**, rồi dừng ở **BF470000 / BFC00904**.
Bổ sung 7 lệnh bảo trì cache theo mô hình bộ nhớ đồng bộ hiện tại;
replay đoạn lỗi tới **BFC00908**. Đọc **PS2_CACHE_BOOT_FIX_V38.md**.
Chưa có cache I/D đầy đủ và chưa xác nhận giao diện BIOS PS2 thật.

Các mục dưới đây là lịch sử cập nhật.

# PSLab — bản 37: vá đọc MCH 16-bit và tăng tốc chạy BIOS

Vá **LHU tại B000F480 / PC 9FC42140**, mở truy cập byte/halfword của các ô MCH.
Tối ưu trace/fetch/bộ nhớ; thêm **Fast run** và log phân biệt vòng chờ Count,
GS chưa bật hiển thị, chưa có gói vẽ. Xem **PS2_BOOT_PROGRESS_V37.md**.

Kiểm tra lệnh lỗi đã tới **9FC42144**. Chưa xác nhận giao diện BIOS PS2 thật.
Bản này không tự tạo menu hoặc ép bật PMODE để thay cho quá trình boot.

Các mục dưới đây là lịch sử cập nhật.

# PSLab — bản 36: MCH, DMA/VIF upload và GS alpha

Vá **B000F410** trong log(10), mở khối **16 ô MCH word** (các ô chưa rõ chỉ
là backing storage). Thêm normal DMA0/1 → VIF, MPG và UNPACK V4-32 giới hạn;
nâng GS với alpha test/blending, COLCLAMP, PABE và destination-alpha test.

Đọc **BIOS_PS2_REGISTER_MAP_V36.md** để xem CPU/GS cần những khối nào,
luồng render và phần còn thiếu; **PS2_MEMORY_DMA_RENDER_V36.md** có phạm vi kỹ thuật.
Replay 12 lệnh cuối tới **9FC41360**. Chưa xác nhận BIOS PS2 boot hoàn chỉnh.

Các mục dưới đây là lịch sử cập nhật.

# PSLab — bản 35: thanh ghi COP1 và lưu/nạp FPU

Vá **SWC1 E7AC00B8 tại 9FC434A4** trong log(9). Thêm 32 thanh ghi FPU,
FCR0/FCR31, LWC1/SWC1, MFC1/MTC1, CFC1/CTC1, MOV.S và nhánh BC1.
Replay 18 lệnh prologue tới **9FC434A8**, kiểm tra ghi dữ liệu thật xuống stack.
Đọc **PS2_COP1_BOOTSTRAP_V35.md**. Đây mới là chuyển dữ liệu COP1;
phép tính số thực và BIOS PS2 boot hoàn chỉnh vẫn chưa được hỗ trợ/xác nhận.

Các mục dưới đây là lịch sử cập nhật.

# PSLab — bản 34: EE serial + thanh ghi VIF0/VIF1

Vá lỗi **B000F100 (SIO_LCR)** trong log(8). Hoàn thiện nhóm **8 vị trí serial**,
thêm **42 thanh ghi VIF** và **7 lệnh cấu hình VIF** qua CPU SQ.
Serial xuất text vào log; VIF có reset/stall, quyền đọc/ghi và payload chia gói.
Xem **PS2_SERIAL_VIF_UPDATE_V34.md** để biết phần hỗ trợ và giới hạn.
Replay 20 lệnh cuối của routine serial tới **9FC43378**. Chưa xác nhận BIOS
PS2 boot hoàn chỉnh; IOP/VU và truyền DMA VIF vẫn chưa thực thi.

Các mục dưới đây là lịch sử cập nhật.

# PSLab — bản 33: mở rộng theo nhóm thanh ghi IOP

Thêm **71 vị trí thanh ghi**, gồm cấu hình bus, ngắt và cấu hình 13 kênh DMA;
thêm **2 MiB RAM IOP + 40 KiB bộ nhớ VU**. Vá lỗi BF801470 trong log(7).
Đọc **PS2_IOP_REGISTER_UPDATE_V33.md** để xem bảng thanh ghi và giới hạn.
Replay 64 lệnh cuối tới **9FC430E0**. Chưa xác nhận BIOS PS2 boot hoàn chỉnh.
IOP/VU chưa thực thi; DMA IOP mới hỗ trợ cấu hình, chưa truyền dữ liệu.

Các mục dưới đây là lịch sử cập nhật.

# PSLab — bản 32.3: vá ghi halfword BF801472

Log(6) chạy 70.143 lệnh và đã ghi thanh ghi thiết lập GS. Bổ sung latch 16-bit
1F801472 với alias/reset/log. Xem **PS2_IOP_HALFWORD_FIX_V323.md**.
Replay 64 lệnh cuối vượt SH tới 9FC430D8; chưa xác nhận boot BIOS hoàn chỉnh.
Các mục dưới đây là lịch sử cập nhật.

# PSLab — bản 32.2: IOP bootstrap byte BF803204

Log mới đã vượt PADDUW và chạy 70067 instruction. Hotfix bổ sung ánh xạ byte
1F803204 qua các alias cho EE BIOS. Đọc **PS2_IOP_BOOT_FIX_V322.md** để xem
phạm vi latch và giới hạn. Replay 64 instruction cuối đi qua LBU tới 9FC42B30.
Chưa có IOP đầy đủ hoặc xác nhận BIOS Sony boot hoàn chỉnh.

# PSLab — bản 32.1: sửa PADDUW khi khởi động BIOS PS2

Hotfix cho opcode **7000D428 tại BFC00874**. Đọc **PS2_PADDUW_FIX_V321.md**.
Thêm sáu lệnh unsigned saturation; replay đúng 39 instruction trong log đã tới BFC00878.
Chưa xác nhận boot hoàn chỉnh BIOS Sony. Các mục bên dưới là lịch sử.

# PSLab — bản 32: PS2 EE / DMA chain / GS

Đọc **PS2_CPU_DMA_GS_V32.md**. Demo mới: **Tools → PS2 EE laboratory → Chain GPU demo → Run continuously**.
CPU tự cấu hình GS, khởi động DMA chain và chờ hoàn tất. GS có tô màu Gouraud cơ bản.
Kèm ROM thử tự tạo `examples/ps2_authored_pipeline_test.bin`, không phải BIOS Sony.
**Chưa xác nhận BIOS PS2 boot hoàn chỉnh hoặc game PS2 chạy được.**
PS1 giữ hotfix v31.1. Các ghi chú bên dưới là lịch sử.

# PSLab — bản 31.1: hotfix nhận phím

Đọc **PS1_KEYBOARD_FIX_V311.md**. Bổ sung đồng bộ phím trực tiếp từ Windows,
thử Cross/Start bằng chuột và hiển thị Host/SIO để chẩn đoán báo cáo không điều khiển được GTA2.
Chưa xác định được nguyên nhân cuối cùng từ ảnh menu; cần thử hotfix trên Windows.

# PSLab — bản 31: PS1 CPU / GPU / bàn phím / CD streaming

C++17 / Win32 / Visual Studio 2022. Đọc **PS1_UPDATE_V31.md** để xem thay đổi,
cách thử GTA 2 và Gran Turismo 2, bảng phím và giới hạn đã biết.
Mục tiêu là tăng khả năng tương thích; **chưa xác nhận GTA 2 qua intro hay GT2 vào đua**.
PS2 giữ mức thử nghiệm v30.1. Các ghi chú phiên bản cũ bên dưới là lịch sử.

# PSLab — bản 30.1

C++17 / Win32 / Visual Studio 2022. PS2 thêm **EE hardware/MMIO, timer và ngắt, normal GIF DMA, CPU branch-likely và GIF PACKED**. Demo mới: Tools → PS2 EE laboratory → **DMA GPU demo → Run continuously**.

Hotfix 30.1 sửa Wired tại BFC00854, thêm COP0/TLB register bank; chưa có MMU translation. Đọc **PS2_WIRED_FIX_V301.md**.

Đọc **PS2_HARDWARE_V30.md** để xem phạm vi và giới hạn. **Chưa xác nhận boot BIOS PS2 hoàn chỉnh hay chạy game PS2 thương mại.**

PS1 giữ audio WSOLA và tối ưu v29; xem **PS1_AUDIO_PERFORMANCE_V29.md**.
Windows: build/startup target **pslab**, chạy pslab.exe, tiêu đề **PSLab v32.2 - PS2 CPU, DMA Chain and GS**.

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
C++ CMake tools for Windows -> File / Open / Folder -> chọn thư mục chứa CMakeLists.txt ->
windows-release -> Startup Item pslab.exe -> Ctrl+F5.
Tiêu đề phải là **PSLab v32.2 - PS2 CPU, DMA Chain and GS**.

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
