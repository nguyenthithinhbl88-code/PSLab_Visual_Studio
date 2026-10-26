# PSLab v32 — EE CPU, DMA chain và GS

Mục tiêu của bản này là làm đường CPU → DMAC → GIF → GS hoạt động sâu hơn để
tiếp tục phát triển khả năng khởi động BIOS PS2. Chưa xác nhận BIOS Sony boot
hoàn chỉnh hoặc game PS2 thương mại chạy được.

Log PS2 gần nhất được kiểm tra là v30.1 với Instructions=0, PC=100000,
DMA2 CHCR=101/QWC=1E: đó là trạng thái demo chưa chạy, không có lỗi BIOS mới
để tái hiện. Các bài kiểm tra dưới đây dùng mã tự tạo.

## CPU và các bank thanh ghi

- Thêm thao tác với HI1/LO1 độc lập: MFHI1/MTHI1/MFLO1/MTLO1,
  MULT1/MULTU1/DIV1/DIVU1; kiểm tra dấu và chia cho 0.
- Thanh ghi 128-bit: PMFHI/PMFLO/PMTHI/PMTLO, PAND/PXOR/POR/PNOR,
  PCPYLD/PCPYUD; bảo toàn dữ liệu khi thanh ghi đích trùng nguồn.
- Packed arithmetic: PADDW/H/B, PSUBW/H/B, PCGTW/H/B, PMAXW/H,
  PCEQW/H/B, xử lý riêng từng lane, không để carry tràn sang lane khác.
- Thanh ghi SA: MFSA/MTSA, MTSAB/MTSAH và QFSRV cho dịch theo byte 0..15.
  Giá trị SA ngoài phạm vi hỗ trợ của QFSRV vẫn báo lỗi rõ ràng.
- LWL/LWR/SWL/SWR và LDL/LDR/SDL/SDR. Store từng phần có kiểm tra trước
  quyền ghi RAM/scratchpad; chưa hỗ trợ unaligned store tới MMIO.
- EI/DI điều khiển EIE theo quyền kernel/EDI/EXL/ERL.
- BC0F/T/FL/TL đọc điều kiện DMAC từ D_PCR và D_STAT để CPU chờ DMA.
  PREF là hint không gây truy cập dữ liệu. CACHE vẫn chưa được mô phỏng.

## DMAC nối CPU với GS

DMA2 ngoài normal mode nay có source chain:

| Tag | Hành vi chính |
|---|---|
| REFE | Đọc payload ở ADDR rồi kết thúc |
| CNT | Payload liền tag, tag kế sau payload |
| NEXT | Payload liền tag, chuyển tag kế tới ADDR |
| REF / REFS | Payload ở ADDR, tag kế liền tag hiện tại |
| CALL | Payload liền tag; ASR0/ASR1 lưu địa chỉ quay về |
| RET | Trở về ASR theo ASP; stack rỗng thì kết thúc |
| END | Payload liền tag rồi kết thúc |

- TADR/MADR/QWC/CHCR.TAG/ASP/ASR0/ASR1 cập nhật khi chuyển gói.
- IRQ trong tag kết hợp CHCR.TIE dừng sau payload; D_STAT báo hoàn tất.
- Resume với QWC còn lại dùng TAG trong CHCR để biết có cần kết thúc.
- Hỗ trợ nguồn/tag trong RAM hoặc scratchpad, wrap scratchpad 16 KiB.
- Tôn trọng DMAC enable, GIF stop/mask và ưu tiên D_PCR.
- PCE trong tag có thể bật/tắt chế độ ưu tiên D_PCR; giá trị reserved báo lỗi.
- Mỗi bus tick chỉ xử lý một tag hoặc một quadword. Chuỗi tag 0-QWC tự vòng
  không khóa host/UI; có thể Stop. Nguồn ngoài RAM báo BUSERR và giữ tiến độ lỗi.
- Chưa có TTE, interleave, MFIFO, stall control hoặc DMA cho các channel khác.
  REFS chỉ hoạt động như REF khi stall control không bật.

## GS

Thêm Gouraud cho đường và tam giác không texture, nội suy RGBA.
Giữ SCISSOR, XYOFFSET, context, FRAME/FBMSK, FBA và PSMCT32/24.
Nội suy/coverage là mô hình chức năng, chưa bit-exact GS thật. Texture,
fog, alpha blending, AA, Z và alpha/depth tests vẫn chưa đầy đủ.

## Thử ngay

Build Release bằng Visual Studio 2022, chạy pslab.exe có tiêu đề v32.
Tools → PS2 EE laboratory → **Chain GPU demo → Run continuously**.
CPU sẽ tự cấu hình PMODE/DISPFB/DISPLAY, khởi động chain REF/END, chuyển
32 quadword qua GIF, vẽ 6 hình (có tam giác nội suy màu), rồi chờ/thoát BC0.

Để kiểm tra đường reset ROM:

1. Trong cửa sổ PS2 chọn Load BIOS (test).
2. Chọn `examples/ps2_authored_pipeline_test.bin` trong gói.
3. Run continuously. ROM tự chép gói từ ROM sang RAM bằng LQ/POR/SQ,
   rồi CPU bật GS và khởi động DMA chain để vẽ các hình trên.

**Tệp này do dự án tự tạo, không phải BIOS Sony và không thay thế BIOS PS2.**
Nó chứng minh đường xử lý ROM → CPU → RAM → DMAC → GIF → GS trong phạm vi đã kiểm tra.

CLI:

```text
pslab_ps2 --chain-demo --steps 1000
pslab_ps2 --bios examples/ps2_authored_pipeline_test.bin --steps 5000
```

Muốn thử BIOS PS2 thật: Load BIOS (test) → chọn BIOS của bạn → Run continuously.
Nếu dừng, Save log ngay, tránh bấm demo hoặc Reset trước khi lưu.
Chưa có MMU translation/cache đầy đủ, IOP, VIF/VU, SPU2 và CDVD;
đây là những phần còn cần hoàn thiện cho quy trình khởi động đầy đủ.

## Kiểm thử

Xem VALIDATION_V32.txt. Các bài mới kiểm tra tag/stack/IRQ, chương trình EE điều khiển
DMA, unaligned memory, MMI và Gouraud. Có ROM tự tạo 4 MiB chạy từ BFC00000
tới 640×448 scanout. Không coi bài ROM này là kiểm thử tương thích BIOS Sony.
Chưa build MSVC hoặc chạy Win32 trong môi trường hiện tại.

Nguồn đối chiếu: PS2tek và mã nguồn PCSX2 được liệt kê trong RESEARCH.md.
Phần bổ sung viết mới trong PSLab; không nhập nguyên core của giả lập khác.
