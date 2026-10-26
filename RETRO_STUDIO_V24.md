# v24 — Retro Studio

Giao diện Win32 được xây lại với tông navy/cyan, sidebar thao tác phiên chơi,
nút Play/Boot nổi bật, màn hình game lớn và thanh phím. Màn chào có ảnh tạo riêng
trong assets/welcome.png; khi Play/nạp BIOS/EXE sẽ chuyển sang đầu ra giả lập thật.
Ảnh minh họa không thay thế framebuffer game.

- Sidebar: Load BIOS, Open disc, Boot disc, Play/Resume, Pause, Keyboard controls,
  Cinema mode, Speed, Audio, Export log.
- File: nạp BIOS/CD/EXE, boot và xuất log.
- View → Show debugger: bật/tắt debugger (mặc định ẩn).
- View → Cinema mode: mở rộng vùng game, ẩn sidebar; bật lại trong menu View.
- Tools: giữ reset/step/Run10k, region/eject, các demo và kiểm tra âm thanh.
- Bàn phím giữ nguyên bản23. Escape tại vùng game sẽ Pause.
- Các số Hz model/MIPS là số đo thật của lõi; không phải FPS game.

Build x64 Release bằng Visual Studio. Tiêu đề PS1 Lab v24 - Retro Studio.
CMake liên kết GDI+ có sẵn trên Windows và tự sao chép assets cạnh ps1_ui.exe.
Khi chuyển EXE sang thư mục khác phải chuyển cả thư mục assets đi cùng. Nếu thiếu
ảnh, màn chào vẫn có nền tối và hướng dẫn. Lõi CPU/GPU/CD/SPU không thay đổi trong
bản UI này. Bộ35 test lõi đã đạt ở bản23; không tuyên bố35 test này xác minh UI.
Đã rà các22 command cũ, owner-draw handlers, vòng đời image/GDI+ và đóng gói asset.
Môi trường hiện tại không có Windows SDK/MSVC: chưa biên dịch hay xác minh hình
chụp cửa sổ Windows thật. Cần kiểm tra layout/font/DPI trên máy chạy.

## Ảnh tạo riêng

Mode: built-in image generation. Asset: assets/welcome.png. Không lấy ảnh game hay
logo Sony. Prompt cuối đã dùng:

Use case: stylized-concept. Create a polished wide landscape digital illustration to embed as the welcome screen in a desktop retro console emulator called PS1 Lab. An original unbranded light-gray mid-1990s disc game console and wired classic digital controller on a dark desk, luminous cyan/teal edge lighting and subtle warm amber accent, smoky navy background, tasteful fine grain, premium editorial 3D illustration. Composition: hardware on right half, left half dark quiet negative space for real app typography. Wide 16:9 landscape. No text, no letters, no logos, no watermark, no recognizable game characters. This is an actual app artwork asset, not a screenshot or UI mockup.
