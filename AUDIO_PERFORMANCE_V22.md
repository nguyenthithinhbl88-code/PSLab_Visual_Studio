# v22 — âm thanh và hiệu năng

Tiêu đề: PS1 Lab v22 - Audio Worker and Performance.

Âm thanh dùng worker riêng, CALLBACK_EVENT và kiểm tra PCM mỗi4ms; không còn phụ thuộc timer giao diện. Queue được chuyển dưới mutex ngắn; nội suy và waveOutWrite nằm ngoài mutex. Có8 buffer512frame, đo tốc độ nguồn sớm hơn, tính cả buffer đang phát, fade128frame khi bắt đầu/khôi phục và chuyển mute. Đóng thiết bị luôn join worker trước khi reset/unprepare. Log thêm audio_host_drops/audio_buffered.

Vẽ RGB dùng bản sao VRAM ngoài mutex. CPU fetch RAM/BIOS qua đường trực tiếp với alignment/IRQ/fault/trace được giữ. Timer được inline; giảm kiểm tra chopping không dùng. MDEC chuẩn bị hệ số một lần mỗi block và bỏ phép nhân hệ số0, giữ kết quả nguyên của thuật toán cũ.

34 bộ test lõi PASS với GCC C++17 -O2 -Wall -Wextra -Werror. Test mới so4000 vector IDCT với thuật toán trước,300000 bước timer và fetch aliases/boundaries/fault. AudioPlayback kiểm tra fade/mute/queue/rate và5giây nguồn nửa tốc độ. Worker WinMM qua mock trên Linux xác minh tự refill không cần UI, restart/join, giải phóng buffer và lỗi thiết bị. Mock không phải kiểm tra SDK/driver Windows thật. Không chạy ASan mới cho bản22.

Probe tổng hợp trước: CPU poll17.24–23.42MIPS, SPU/timer14.30–20.56MIPS, MDEC DC0.42–0.62triệu block/s, dense0.44–0.59triệu. Lần sau sửa:17.92MIPS,17.88MIPS, DC2.15triệu, dense0.71triệu. Máy chủ có dao động lớn: không kết luận CPU nhanh hơn theo phần trăm từ phép đo này. IDCT có cải thiện trong probe; không quy đổi thành FPS GTA. tools/performance_probe.cpp cho phép đo lại cùng máy/build.

Ảnh người dùng xác nhận v21.1 đã vào menu/màn chơi GTA1. Bản22 chưa chạy BIOS/GTA hoặc nghe thiết bị Windows tại đây. Âm thanh thích ứng vẫn đổi cao độ khi lõi chạy chậm, chưa time-stretch giữ cao độ. Buffer8block có thể tăng độ trễ; không hứa hết underrun hay fullspeed. GTE/MDEC/timing và lỗi hình ảnh vẫn còn giới hạn cũ.

Giải nén thư mục mới, build x64 Release, Nạp BIOS → Mount CUE → Boot Game CD → Run. Giữ Speed1x/SoundOn. So cùng cảnh bằng VideoHz/MIPS và audio_underruns/audio_ratio; Save log khi có vấn đề.

Tham khảo Microsoft: https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutopen và https://learn.microsoft.com/en-us/windows/win32/multimedia/using-an-callback-to-process-driver-messages . Implementation tự viết.
