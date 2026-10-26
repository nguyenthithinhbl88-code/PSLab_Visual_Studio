# PSLab v30 — EE hardware, GIF DMA và nhiều thanh ghi hệ thống

C++17 / Visual Studio 2022. Giữ phần PS1/audio WSOLA v29. PS2 vẫn là core thử nghiệm: **chưa xác nhận boot BIOS Sony tới Browser/logo hoặc chạy game PS2 thương mại**.

## Có gì thực sự chạy

- DMA GIF **normal mode**: đọc quadword RAM, gửi qua parser GIF đến GS, tăng MADR, giảm QWC, tắt STR và đặt D_STAT sau khi truyền xong. Một QW mỗi bus tick trong mô hình; không chạy một vòng DMA khổng lồ trong thao tác ghi thanh ghi của UI.
- D_ENABLEW/D_ENABLER dùng chung trạng thái, reset 0x1201; bit disable dừng/tiếp tục tiến độ DMA. D_CTRL DMAE và D_PCR priority được dùng cho kênh GIF.
- Timer 0..3 đếm theo bus, /16, /256 hoặc HBlank. Compare/reset-on-compare, overflow, cờ W1C và ngắt theo cạnh của cờ.
- INTC: acknowledge từng bit và mask toggle. D_STAT: clear status, toggle mask, BUSERR không bị mask. Pending được đưa lên COP0.Cause.
- Ngắt EE có kiểm tra IE/EIE/EXL/ERL và mask, lưu EPC/BD, chọn vector thường/bootstrap, ERET trở về. Count/Compare đặt pending; MTC0 Compare xóa pending timer.
- HBlank/VBlank tạo sự kiện cho timer/INTC/CSR GS. Thời gian mô phỏng vẫn xấp xỉ, chưa phải đồng hồ máy thật.
- CPU thêm 8 dạng REGIMM branch, bốn nhóm branch-likely, annul delay slot khi không taken, link r31, DSLLV/DSRLV/DSRAV và LWU. Thêm hai vùng RAM alias uncached/accelerated.
- GIF PACKED mở rộng RGBA, STQ/Q, UV, XYZ/XYZF, FOG, TEX0/CLAMP; ADC chuyển vertex sang dạng không drawing-kick. Giữ A+D, REGLIST và IMAGE của bản trước.

## Các bank mới

Có **89 địa chỉ word MMIO** được bộ hardware mới nhận dạng. Số địa chỉ không đồng nghĩa 89 thiết bị đã mô phỏng hoàn chỉnh.

| Bank | Địa chỉ/phạm vi | Hiện thực trong v30 |
|---|---|---|
| Timer 0..3 | 10000000 + N*800: COUNT/MODE/COMP; HOLD chỉ T0/T1 | 14 thanh ghi, đếm/compare/overflow/IRQ chức năng; HOLD chỉ latch |
| 10 kênh DMA | 10008000, 9000, A000, B000, B400, C000, C400, C800, D000, D400 | CHCR/MADR/QWC/TADR lưu trạng thái; ASR0/1 cho 0..2, SADR cho 8..9; chỉ GIF normal có truyền dữ liệu |
| DMAC global | 1000E000..1000E060 cách 10h | CTRL/STAT/PCR có phần điều khiển; SQWC/RBSR/RBOR/STADR latch, chưa có interleave/MFIFO/stall engine |
| DMAC enable | 1000F520 / 1000F590 | Đọc trạng thái / ghi enable, dùng để pause/resume |
| INTC | 1000F000 / 1000F010 | W1C status, toggle mask, nối ngắt EE |
| SIF | 1000F200/210/220/230/240/260 | EE-owned MSCOM; SMCOM không cho EE ghi; MSFLAG set, SMFLAG clear; CTRL/BD6 một phần |
| GIF | 10003000/10/20/40/50/60/70/80/90/A0 | CTRL/MODE/STAT và snapshot tag/counter; giới hạn bên dưới |

MMIO hệ thống mới hiện chỉ hỗ trợ word 32 bit aligned. Gaps, width khác hoặc thiết bị khác chưa hỗ trợ vẫn báo lỗi. GS privileged vẫn giữ đường 32/64 bit riêng.

## Demo có đường truyền DMA thật

Windows: Tools → PS2 EE laboratory → **DMA GPU demo → Run continuously**.

Sau khi chạy, GS Screen hiện demo hình học tương tự GPU demo cũ, nhưng packet được gửi từ RAM qua DMA channel 2. Diagnostic chuẩn:

```
DMA QW=30
DMA2 CHCR=1 MADR=2e0 QWC=0
GS primitives=6
```

Demo harness khởi tạo thiết bị và packet do dự án tự viết, CPU chạy vòng lặp trong lúc engine DMA tiến từng bus tick. Không phải chạy game hay BIOS Sony.

CLI:

```powershell
.\pslab_ps2.exe --dma-demo --steps 1000
.\pslab_ps2.exe --bios D:\BIOS\your_ps2.bin --steps 100000 *> ps2_log.txt
```

Thử BIOS thật: kiểm tra tiêu đề **v30**, Load BIOS (test) → Run continuously → Save log nếu dừng. Nếu chờ SIF, đọc các giá trị SIF trong log: hiện chưa có IOP chạy để trả lời.

## Giới hạn cần biết khi đánh giá

- Một lệnh EE thành công = một tick EE; hai tick EE = một bus tick. Count vẫn tăng theo lệnh, không phải chu kỳ R5900 chính xác. Ngắt entry được đếm riêng, không tính như lệnh thực thi.
- Timing NTSC/PAL là mô hình đơn giản; HBlank dùng khoảng cuối 1/5 dòng, gate timer chỉ xấp xỉ. Các gate mode reset theo cạnh chưa có đầy đủ start/stop như phần cứng. HOLD chưa được nguồn SBUS thật chốt tự động.
- GS IRQ được nối bằng cạnh của đường IRQ tổng; đồng thời nhiều nguồn có thể bị gộp. Chưa mô phỏng chính xác từng pulse.
- DMA chain, interleave, MFIFO, scratchpad DMA và các kênh khác chưa chạy. Start chúng dừng lỗi rõ ràng. Latch địa chỉ của những kênh đó không tự thực hiện I/O.
- DMA ngoài RAM đặt BUSERR rồi dừng chẩn đoán; QW lỗi giữ nguyên tiến độ. Parser GS lỗi cũng giữ first_fault, không giả hoàn tất.
- GIF mask/stop tạm dừng DMA ngay, chưa có FIFO 16 QW và quy tắc mask trì hoãn đầy đủ. CPU SQ vào FIFO khi bị mask/stop dừng lỗi vì chưa có hàng đợi stall.
- GIF STAT là trạng thái chức năng rút gọn. TAG/CNT/P3CNT/P3TAG là snapshot của parser hiện tại, chưa có PATH1/2 hoặc context bị ngắt. Trường VU address của CNT bằng 0; snapshot cho phép đọc cả ngoài lúc pause.
- PACKED STQ/Q chỉ vận chuyển bit float; bộ vẽ vẫn chưa hỗ trợ texture/Gouraud/depth/blend đầy đủ. Không được hiểu thêm descriptor là đã có renderer hoàn chỉnh.
- SIF chưa có IOP, RPC, module loader hay tín hiệu IOP boot-complete. Không tự đặt SMFLAG=40000 để ép BIOS đi qua.
- TLB, cache, FPU, VU, IOP, CDVD, SPU2 còn thiếu. Không có căn cứ hứa boot BIOS đầy đủ hay chạy GTA PS2 từ bản này.
- Bản v29 PS1/audio được giữ; không sửa clock hay bỏ lệnh GPU PS1 trong cập nhật này.

## Kiểm thử

- 46 bộ kiểm tra core PS1/PS2 bằng GCC C++17 -O2 -Wall -Wextra -Werror.
- Bộ EE hardware mới: bank/mask/alias, timer IRQ edge/W1C/divider/HBlank/VBlank, DMA pause/stop/completion/bus error, SIF ownership, IRQ bootstrap/BD/EPC/ERET/Compare.
- Branch tests: 8 REGIMM × ba giá trị âm/0/dương; bốn branch-likely; shift và LWU.
- PACKED tests: RGBA/Q/STQ/UV/XYZ/XYZF/FOG/TEX/CLAMP, ADC và descriptor reserved.
- Bộ hardware mới chạy thêm ASan/UBSan (LeakSanitizer tắt do môi trường). CLI DMA demo đối chiếu 30 QW/6 primitive.
- Chưa có BIOS/game PS2 thật ở môi trường phát triển; chưa build/chạy Win32 bằng MSVC tại đây. Cần người dùng thử lại BIOS để biết điểm dừng tiếp theo.

## Tài liệu và việc dùng mã nguồn bên ngoài

Đã đối chiếu ps2tek, mã register/interrupt/counter/branch của PCSX2 và xem thông tin/giấy phép Play!. Không ghép nguyên core của giả lập khác. Logic mới được viết trong dự án và giữ LICENSE MIT hiện tại; không thêm mã GPL của PCSX2 hoặc mã BSD của Play! vào cây nguồn này.

- ps2tek: https://psi-rockin.github.io/ps2tek/index.html
- PCSX2 INTC/SIF write semantics: https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/HwWrite.cpp/
- PCSX2 counters: https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/Counters.cpp/
- PCSX2 branch/link/likely: https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/Interpreter.cpp/
- PCSX2 opcode tables: https://github.com/PCSX2/pcsx2/blob/master/pcsx2/R5900OpcodeTables.cpp
- PCSX2 license: https://github.com/PCSX2/pcsx2/blob/master/COPYING.GPLv3
- Play! repository: https://github.com/jpd002/Play-
- Play! license: https://github.com/jpd002/Play-/blob/master/License.txt

Không lấy kiến trúc HLE BIOS của Play! thay cho luồng BIOS thật đang phát triển ở PSLab. Việc đọc/đối chiếu mã không phải bằng chứng tương thích game của core này.
