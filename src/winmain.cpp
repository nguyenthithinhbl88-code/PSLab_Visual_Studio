#include <windows.h>
#include <commdlg.h>
#include <gdiplus.h>
#include "ui_theme.hpp"
#include "core.hpp"
#include "display.hpp"
#include "runner.hpp"
#include "audio_win32.hpp"
#include "keyboard_pad.hpp"
#include "ps2_window.hpp"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <memory>
#include <chrono>

namespace {
std::unique_ptr<ps1::Bus> bus;
std::unique_ptr<ps1::Cpu> cpu;
std::mutex machine_mutex;
std::unique_ptr<ps1::Runner> runner;
ps1::AudioOutput audio_output;
std::vector<uint32_t> presentation_pixels;
ps1::Gpu presentation; // UI-owned VRAM snapshot; convert RGB after releasing the core lock
bool limit_speed=true,muted=false;
uint64_t painted_words=~uint64_t(0),painted_display=~uint64_t(0),painted_video=~uint64_t(0);
HWND output,screen,pad_hint;
ps1::KeyboardPad keyboard;
std::array<HWND,22> buttons{};
bool large_view=false,debug_visible=false,show_welcome=true;
HFONT ui_font=nullptr,title_font=nullptr,small_font=nullptr;
HBRUSH background_brush=nullptr,panel_brush=nullptr;
std::unique_ptr<Gdiplus::Image> welcome_art;
void paint_shell(HWND w,HDC dc);

void layout(HWND w){
 RECT r{};GetClientRect(w,&r);const int width=r.right,height=r.bottom;
 for(auto button:buttons)ShowWindow(button,SW_HIDE);
 const int sidebar=large_view?0:240,x=large_view?24:288;
 const int stage_width=std::max(1,width-x-24),stage_height=std::max(1,height-218);
 const unsigned visible[]={1,11,15,4,5,21,13,18,19,10};
 for(unsigned n=0;n<10&&!large_view;n++){auto button=buttons[visible[n]];ShowWindow(button,SW_SHOW);MoveWindow(button,36,144+int(n)*43,192,35,TRUE);}
 const int debug_width=debug_visible?std::min(420,stage_width/2):0;
 ShowWindow(output,debug_visible?SW_SHOW:SW_HIDE);
 if(debug_visible)MoveWindow(output,width-24-debug_width,112,debug_width,stage_height,TRUE);
 MoveWindow(screen,x,112,std::max(1,stage_width-(debug_width?debug_width+16:0)),stage_height,TRUE);
 MoveWindow(pad_hint,x,height-94,std::max(1,width-x-24),34,TRUE);
 (void)sidebar;InvalidateRect(w,nullptr,FALSE);InvalidateRect(screen,nullptr,FALSE);
}
HFONT font;
uint32_t entry=0;
bool running=false;
ULONGLONG last_refresh=0;
bool exe_loaded=false,has_bios=false;
ULONGLONG perf_time=0;
uint64_t perf_frames=0,perf_instructions=0;
double video_hz=0,core_mips=0;
void start_perf(){std::lock_guard<std::mutex> lock(machine_mutex);perf_time=GetTickCount64();perf_frames=bus->gpu.video_frames;perf_instructions=cpu->instructions;video_hz=core_mips=0;}
void sample_perf(){
 const auto now=GetTickCount64();if(now-perf_time<1000)return;
 std::lock_guard<std::mutex> lock(machine_mutex);
 const double seconds=double(now-perf_time)/1000;
 if(bus->gpu.video_frames>=perf_frames&&cpu->instructions>=perf_instructions){video_hz=double(bus->gpu.video_frames-perf_frames)/seconds;core_mips=double(cpu->instructions-perf_instructions)/seconds/1000000;}
 perf_time=now;perf_frames=bus->gpu.video_frames;perf_instructions=cpu->instructions;
}
void update_pad(HWND w){
 const bool focused=GetAncestor(GetForegroundWindow(),GA_ROOT)==w;
 if(!focused)keyboard.release();
 std::lock_guard<std::mutex> lock(machine_mutex);
 bus->sio[0].connected=true;bus->sio[0].buttons=keyboard.buttons(focused);
}
std::array<uint32_t,32> initial_regs{};
std::wstring last_fault;
std::wstring status=L"San sang. GTA1 da vao man choi tren may nguoi dung; dang toi uu.";
void refresh(){
 // Hidden diagnostics must not format every register or send Win32 messages under the core mutex.
 if(!debug_visible){InvalidateRect(GetParent(screen),nullptr,FALSE);if(screen&&!running)InvalidateRect(screen,nullptr,FALSE);return;}
 std::unique_lock<std::mutex> lock(machine_mutex);
 std::wostringstream s;
 s<<L"PS1 LAB | CPU DEBUGGER\r\n"<<status<<L"\r\n\r\n";
 s<<L"Video Hz (model): "<<std::fixed<<std::setprecision(2)<<video_hz<<L" | CPU MIPS: "<<core_mips<<L"\r\n";
 s<<L"PC: "<<std::hex<<std::setfill(L'0')<<std::setw(8)<<cpu->pc<<L"    Next: "<<std::setw(8)<<cpu->next_pc<<L"\r\n\r\n";
 s<<L"Boot path: "<<(bus->direct_exe_boot?L"DIRECT EXE (no BIOS initialization)":L"Machine / BIOS reset")<<L"\r\n";
 s<<L"IRQ exceptions: "<<std::dec<<cpu->interrupts<<L" | Reserved: "<<cpu->reserved_exceptions<<L"\r\n";
 s<<L"GTE commands: "<<cpu->gte.commands<<L"\r\n";
 s<<L"MDEC blocks: "<<bus->mdec.macroblocks<<L" | Status: "<<std::hex<<bus->mdec.status()<<std::dec<<L"\r\n";
 if(cpu->first_fault.valid)s<<L"First CPU fault: "<<std::dec<<cpu->first_fault.code<<L" PC="<<std::hex<<cpu->first_fault.pc<<L" opcode="<<cpu->first_fault.opcode<<L"\r\n";
 s<<std::hex;
 s<<L"HI: "<<std::setw(8)<<cpu->hi<<L" LO: "<<std::setw(8)<<cpu->lo<<L"\r\n";
 s<<L"SR: "<<std::setw(8)<<cpu->cop0[12]<<L" Cause: "<<std::setw(8)<<cpu->cop0[13]<<L" EPC: "<<std::setw(8)<<cpu->cop0[14]<<L"\r\n\r\n";
 for(unsigned i=0;i<32;i++){
  s<<L"R"<<std::dec<<std::setfill(L'0')<<std::setw(2)<<i<<L"  "<<std::hex<<std::setw(8)<<cpu->r[i]<<((i%2==1)?L"\r\n":L"    ");
 }
 s<<L"\r\nInstructions: "<<std::dec<<cpu->instructions<<L" POST: "<<std::hex<<unsigned(bus->post)<<L"\r\n";
 s<<L"\r\nPAD1 buttons (active-low): "<<std::hex<<bus->sio[0].buttons<<L" | Keyboard connected\r\n";
 s<<L"\r\nDMA words: "<<std::dec<<bus->dma_words<<L"\r\n";
 const auto disc_info=bus->cd.disc.description();s<<L"Disc: "<<std::wstring(disc_info.begin(),disc_info.end())<<L"\r\n";
 s<<L"CD sectors: "<<std::dec<<bus->cd.sectors<<L" Read: "<<bus->cd.sectors_read<<L" Commands: "<<bus->cd.commands<<L"\r\n";
 s<<L"Video frames: "<<bus->gpu.video_frames<<L" Line: "<<bus->gpu.video_line<<L" VBlank: "<<bus->gpu.vblank_events<<L"\r\n";
 s<<L"GPU polygons: "<<bus->gpu.polygons<<L" Sprites: "<<bus->gpu.rectangles<<L"\r\n";
 s<<L"SPU writes: "<<bus->spu.writes<<L" | Voices: "<<bus->spu.active_voices()<<L" | Samples: "<<bus->spu.generated_frames<<L"\r\n";
 s<<L"Audio: "<<(audio_output.ready()?L"44100 Hz stereo / WSOLA":L"device closed")<<L" | MMCSS: "<<audio_output.multimedia_scheduled()<<L" | Underruns: "<<audio_output.underruns()<<L" | Tempo / WSOLA: "<<audio_output.ratio()<<L" | Buffered: "<<audio_output.buffered_frames()<<L" | Dropped: "<<(bus->spu.dropped_frames+audio_output.dropped())<<L" | Pace: "<<(limit_speed?L"1x model":L"Max")<<L"\r\n";
 s<<L"\r\nRAM[0x100]: "<<std::dec<<bus->read32(256)<<L"\r\n\r\n";
 const auto view=ps1::display_view(bus->gpu);
 s<<L"Display source: "<<view.x<<L","<<view.y<<L" "<<view.width<<L"x"<<view.height<<L" | Fit 4:3\r\n";
 s<<L"GPU: partial. SPU: ADPCM/ADSR stereo (partial). CD-ROM: ISO/BIN/CUE subset.\r\nBIOS co the dung o lenh hoac MMIO chua ho tro.\r\nPad: mui ten, Z/X/A/S, Enter/Shift. Homebrew direct boot; GTA1 da vao man choi tren may nguoi dung; tuong thich chua day du.";
 std::string tty=bus->tty;s<<L"\r\nTTY: "<<std::wstring(tty.begin(),tty.end());
 const auto text=s.str();lock.unlock();
 SetWindowTextW(output,text.c_str());
 InvalidateRect(GetParent(screen),nullptr,FALSE);
 if(screen&&!running)InvalidateRect(screen,nullptr,FALSE);
}
void stop(HWND w){keyboard.release();update_pad(w);running=false;KillTimer(w,1);if(runner){runner->stop();runner.reset();}audio_output.close();}
void start_run(HWND w){
 show_welcome=false;
 start_perf();update_pad(w);
 {std::lock_guard<std::mutex> lock(machine_mutex);bus->spu.audio.clear();}
 audio_output.set_muted(muted);
 const bool audio_ok=audio_output.open(bus->spu,machine_mutex);
 try{runner=std::make_unique<ps1::Runner>(*cpu,machine_mutex);runner->start(limit_speed);}
 catch(const std::exception& e){stop(w);std::string message=e.what();status=L"Run: "+std::wstring(message.begin(),message.end());refresh();return;}
 running=SetTimer(w,1,16,nullptr)!=0;
 if(!running){stop(w);status=L"Khong tao duoc UI timer.";}
 else{SetFocus(screen);status=audio_ok?L"CPU worker dang chay; SPU stereo 44.1 kHz.":L"CPU dang chay; khong mo duoc thiet bi am thanh Windows.";}
 refresh();
}
void advance(HWND w,unsigned n){
 show_welcome=false;
 try{for(unsigned i=0;i<n;i++){cpu->step();if(cpu->bios_system_error)throw std::runtime_error(cpu->failure_summary());}status=L"Da thuc thi lenh.";}
 catch(const std::exception& e){stop(w);std::string message=e.what();status=L"Dung: "+std::wstring(message.begin(),message.end());last_fault=status;}
 refresh();
}
void loadBios(HWND w){
 wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=file;o.nMaxFile=32768;
 o.lpstrFilter=L"BIOS (*.bin)\0*.bin\0All files\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&o))return;
 stop(w);
 try {
  std::ifstream f(std::filesystem::path(file),std::ios::binary|std::ios::ate);
  if(!f||f.tellg()!=std::streamoff(512*1024))throw std::runtime_error("BIOS must be exactly 512 KiB");
  std::vector<uint8_t> image(512*1024);f.seekg(0);
  if(!f.read(reinterpret_cast<char*>(image.data()),image.size()))throw std::runtime_error("Read failed");
  show_welcome=false;ps1::boot_bios(*bus,*cpu,image);exe_loaded=false;has_bios=true;entry=0xbfc00000;last_fault.clear();
  status=bus->cd.sectors?L"BIOS loaded; dia van duoc giu. Chon Boot Game CD -> Run.":L"BIOS loaded. Bam Run de thu BIOS, hoac Mount CD -> Boot Game CD -> Run.";
 }catch(const std::exception&){status=L"Khong nap duoc BIOS. Can tep 512 KiB hop le.";}
 refresh();
}
void openExe(HWND w){
 wchar_t file[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=file;o.nMaxFile=32768;
 o.lpstrFilter=L"PS-X EXE (*.exe;*.ps-exe)\0*.exe;*.ps-exe\0All files\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&o))return;
 stop(w);
 try{
  std::ifstream f(std::filesystem::path(file),std::ios::binary|std::ios::ate);
  auto size=f.tellg();if(!f||size<0x800||size>4*1024*1024)throw std::runtime_error("EXE file size invalid (max 4 MiB)");
  std::vector<uint8_t> bytes(static_cast<size_t>(size));f.seekg(0);
  if(!f.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())))throw std::runtime_error("EXE read failed");
  auto fresh=std::make_unique<ps1::Bus>();auto freshCpu=std::make_unique<ps1::Cpu>(*fresh);
  show_welcome=false;ps1::load_exe(*fresh,*freshCpu,bytes);
  cpu.reset();bus=std::move(fresh);cpu=std::move(freshCpu);entry=cpu->pc;initial_regs=cpu->r;exe_loaded=true;has_bios=false;
  status=L"PS-X EXE loaded. Bam Run. Khong co BIOS services; chi homebrew tu khoi tao.";
 }catch(const std::exception& e){std::string text=e.what();status=L"EXE: "+std::wstring(text.begin(),text.end());}
 refresh();
}
void mountIso(HWND w){
 wchar_t path[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=path;o.nMaxFile=32768;o.lpstrFilter=L"PS1 disc (*.cue;*.bin;*.iso)\0*.cue;*.bin;*.iso\0All files\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&o))return;stop(w);
 try{bus->cd.mount(std::filesystem::path(path));status=L"Disc mounted. Game thuong mai: Nap BIOS -> Boot Game CD -> Run. Homebrew EXE chi cho chuong trinh tu khoi tao.";}
 catch(const std::exception&e){std::string msg=e.what();status=std::wstring(msg.begin(),msg.end());}refresh();
}
void paint_shell(HWND w,HDC dc){
 RECT client{};GetClientRect(w,&client);FillRect(dc,&client,background_brush);
 ps1ui::label(dc,L"PSLab",RECT{28,16,250,61},title_font,ps1ui::text);
 ps1ui::label(dc,L"PS1  +  PS2 LAB  /  v30.1",RECT{30,62,270,84},small_font,ps1ui::accent);
 ps1ui::label(dc,running?L"●  RUNNING":L"●  READY",RECT{client.right-200,24,client.right-24,54},ui_font,ps1ui::accent,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 if(!large_view)ps1ui::box(dc,RECT{24,108,264,client.bottom-24},ps1ui::panel,ps1ui::border);
 if(!large_view){
  ps1ui::label(dc,L"SESSION",RECT{38,111,230,139},small_font,ps1ui::muted);
  ps1ui::label(dc,L"DIGITAL PAD 1",RECT{38,596,234,624},small_font,ps1ui::accent);
  ps1ui::label(dc,L"Click the game to focus.\nZ confirms. Enter starts.\nAlt-Tab releases all keys.",RECT{38,628,240,708},small_font,ps1ui::muted,DT_LEFT|DT_WORDBREAK);
 }
 const int x=large_view?24:288;
 std::wostringstream metrics;metrics<<std::fixed<<std::setprecision(1)<<video_hz<<L" Hz model    /    "<<core_mips<<L" MIPS    /    AUDIO "<<(muted?L"OFF":L"ON");
 ps1ui::label(dc,metrics.str().c_str(),RECT{x,78,client.right-24,108},small_font,ps1ui::muted);
 ps1ui::label(dc,status.c_str(),RECT{x,client.bottom-54,client.right-24,client.bottom-12},small_font,ps1ui::muted,DT_LEFT|DT_WORDBREAK);
}
LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 switch(m){
 case WM_CREATE:{
  const wchar_t* labels[]={L"CPU demo",L"Load BIOS",L"Reset CPU",L"Step",L"Play / Resume",L"Pause",L"GPU demo",L"Polygon",L"Open EXE",L"Run 10k",L"Export log",L"Open disc",L"Homebrew EXE",L"Cinema mode",L"Texture demo",L"Boot disc",L"Region CD",L"Eject disc",L"Speed: 1x",L"Audio: On",L"SPU test",L"Keyboard controls"};
  for(int i=0;i<22;i++){auto b=CreateWindowW(L"BUTTON",labels[i],WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,16+(i%8)*120,16+(i/8)*42,110,36,w,reinterpret_cast<HMENU>(INT_PTR(101+i)),nullptr,nullptr);buttons[i]=b;SendMessageW(b,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);}
  pad_hint=CreateWindowW(L"STATIC",L"KEYBOARD   Arrows  D-pad     Z / X  Cross / Circle     A / S  Square / Triangle\nEnter  Start     Shift  Select     Q / W  L1 / R1     E / R  L2 / R2",WS_CHILD|WS_VISIBLE,16,90,1300,24,w,nullptr,nullptr,nullptr);
  output=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_READONLY,16,108,740,400,w,nullptr,nullptr,nullptr);
  screen=CreateWindowW(L"PS1Vram",L"",WS_CHILD|WS_VISIBLE,780,108,640,480,w,nullptr,GetModuleHandleW(nullptr),nullptr);
  font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FIXED_PITCH,L"Consolas");
  for(auto button:buttons)SendMessageW(button,WM_SETFONT,reinterpret_cast<WPARAM>(ui_font),TRUE);
  SendMessageW(pad_hint,WM_SETFONT,reinterpret_cast<WPARAM>(small_font),TRUE);
  HMENU menu=CreateMenu(),file=CreatePopupMenu(),view=CreatePopupMenu(),tools=CreatePopupMenu();
  AppendMenuW(file,MF_STRING,102,L"Load BIOS...");AppendMenuW(file,MF_STRING,112,L"Open disc...");AppendMenuW(file,MF_STRING,116,L"Boot disc");AppendMenuW(file,MF_STRING,109,L"Open homebrew EXE...");AppendMenuW(file,MF_STRING,111,L"Export diagnostic log...");AppendMenuW(file,MF_STRING,124,L"Exit");
  AppendMenuW(view,MF_STRING,123,L"Show debugger");AppendMenuW(view,MF_STRING,114,L"Cinema mode");AppendMenuW(view,MF_STRING,122,L"Keyboard controls");
  const unsigned ids[]={103,104,110,117,118,101,107,108,115,121,113};
  const wchar_t* names[]={L"Reset CPU",L"Single step",L"Run 10k",L"CD region",L"Eject disc",L"CPU demo",L"GPU demo",L"Polygon demo",L"Texture demo",L"Audio test",L"Direct homebrew boot"};
  for(unsigned n=0;n<11;n++)AppendMenuW(tools,MF_STRING,ids[n],names[n]);
  AppendMenuW(tools,MF_SEPARATOR,0,nullptr);AppendMenuW(tools,MF_STRING,125,L"PS2 EE laboratory (experimental)...");
  AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(file),L"File");AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(view),L"View");AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(tools),L"Tools");SetMenu(w,menu);
  SendMessageW(output,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);layout(w);refresh();return 0;}
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:{PAINTSTRUCT paint{};auto dc=BeginPaint(w,&paint);paint_shell(w,dc);EndPaint(w,&paint);return 0;}
 case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:{auto dc=reinterpret_cast<HDC>(wp);SetTextColor(dc,ps1ui::muted);SetBkColor(dc,ps1ui::background);return reinterpret_cast<LRESULT>(background_brush);}
 case WM_DRAWITEM:{auto item=reinterpret_cast<DRAWITEMSTRUCT*>(lp);if(item->CtlType!=ODT_BUTTON)break;
  const bool primary=item->CtlID==105||item->CtlID==116,pressed=item->itemState&ODS_SELECTED;
  RECT r=item->rcItem;ps1ui::box(item->hDC,r,primary?(pressed?RGB(47,164,146):ps1ui::accent):(pressed?RGB(38,56,68):ps1ui::panel),primary?ps1ui::accent:ps1ui::border,10);
  wchar_t text[128]{};GetWindowTextW(item->hwndItem,text,128);ps1ui::label(item->hDC,text,r,ui_font,primary?ps1ui::background:ps1ui::text,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  if(item->itemState&ODS_FOCUS){InflateRect(&r,-4,-4);DrawFocusRect(item->hDC,&r);}return TRUE;}
 case WM_ACTIVATEAPP:if(!wp){keyboard.release();update_pad(w);}return 0;
 case WM_SIZE:if(output&&screen)layout(w);return 0;
 case WM_GETMINMAXINFO:{auto p=reinterpret_cast<MINMAXINFO*>(lp);p->ptMinTrackSize={1080,840};return 0;}
 case WM_COMMAND:
  switch(LOWORD(wp)){
   case 123:debug_visible=!debug_visible;CheckMenuItem(GetMenu(w),123,MF_BYCOMMAND|(debug_visible?MF_CHECKED:MF_UNCHECKED));layout(w);refresh();break;
   case 124:DestroyWindow(w);break;
   case 125:stop(w);open_ps2_window(w);break;
   case 122:{keyboard.release();update_pad(w);MessageBoxW(w,L"Digital Pad 1 - ban phim\n\nMui ten: D-pad\nZ: Cross (X) - chon trong menu GTA\nX: Circle (O)\nA: Square\nS: Triangle\nEnter: Start / Pause trong game\nShift: Select\nQ / W: L1 / R1\nE / R: L2 / R2\n\nBam vao man hinh game truoc khi choi. Giu nhieu phim cung luc duoc.\nAlt-Tab se nha tat ca nut. Day la digital pad, chua co analog/rung.",L"PS1 Keyboard Pad",MB_OK);SetFocus(screen);break;}

   case 115:{stop(w);exe_loaded=false;entry=0;const auto n=ps1::install_texture_demo(*bus);cpu->reset(0);advance(w,n);status=L"Texture demo: CPU upload -> textured quads, modulation, sprite va blend.";refresh();break;}
   case 114:large_view=!large_view;SetWindowTextW(buttons[13],large_view?L"Standard view":L"Cinema mode");layout(w);break;
   case 101:stop(w);exe_loaded=false;entry=0;ps1::install_demo(*bus);cpu->reset(entry);advance(w,4);status=L"Demo hoan tat: 7 + 5 = 12. Xem RAM[0x100].";refresh();break;
   case 112:mountIso(w);break;
   case 113:{stop(w);try{ps1::boot_disc(*bus,*cpu,true);entry=cpu->pc;initial_regs=cpu->r;exe_loaded=true;last_fault.clear();start_perf();status=L"Homebrew EXE loaded. KHONG co kernel BIOS. Game thuong mai: Nap BIOS -> Boot Game CD -> Run.";}catch(const std::exception&e){std::string msg=e.what();status=std::wstring(msg.begin(),msg.end());}refresh();break;}
   case 116:{stop(w);try{if(!has_bios)throw std::runtime_error("Nap BIOS truoc, sau do Mount CD");ps1::boot_disc(*bus,*cpu,false);exe_loaded=false;entry=cpu->pc;last_fault.clear();start_perf();status=L"BIOS reset voi dia da mount. Bam Run. Retail boot chua xac nhan.";}catch(const std::exception&e){std::string msg=e.what();status=std::wstring(msg.begin(),msg.end());}refresh();break;}
   case 117:{stop(w);bus->cd.region=(bus->cd.region+1)%5;const wchar_t* names[]={L"Auto (BOOT prefix)",L"USA",L"Europe",L"Japan",L"Unlicensed"};status=L"GetID region emulated: "+std::wstring(names[bus->cd.region]);refresh();break;}
   case 121:stop(w);exe_loaded=false;entry=0;ps1::install_spu_demo(*bus);cpu->reset(0);start_run(w);status=L"Original SPU test: hai tan so trai/phai; khong can BIOS.";refresh();break;
   case 119:{bool resume=running;stop(w);limit_speed=!limit_speed;SetWindowTextW(buttons[18],limit_speed?L"Speed: 1x":L"Speed: Max");if(resume)start_run(w);else refresh();break;}
   case 120:muted=!muted;audio_output.set_muted(muted);SetWindowTextW(buttons[19],muted?L"Audio: Off":L"Audio: On");break;
   case 118:stop(w);bus->cd.eject();status=L"Disc ejected.";refresh();break;
   case 111:{
    stop(w);wchar_t path[32768]=L"ps1_bios_log.txt";OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=path;o.nMaxFile=32768;o.lpstrFilter=L"Text log\0*.txt\0";o.lpstrDefExt=L"txt";o.Flags=OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR;
    if(GetSaveFileNameW(&o)){std::ofstream f(std::filesystem::path(path),std::ios::binary);std::string fault;for(wchar_t ch:last_fault)fault.push_back(ch<=127?static_cast<char>(ch):'?');f<<fault<<"\nMIPS="<<core_mips<<" VideoHz="<<video_hz<<" paced="<<limit_speed<<" audio_underruns="<<audio_output.underruns()<<" audio_tempo="<<audio_output.ratio()<<" audio_host_drops="<<audio_output.dropped()<<" audio_buffered="<<audio_output.buffered_frames()<<" audio_mmcss="<<audio_output.multimedia_scheduled()<<" audio_error="<<audio_output.error()<<"\n"<<cpu->diagnostic();status=f?L"Da luu diagnostic log.":L"Khong luu duoc log.";refresh();}break;}
   case 109:openExe(w);break;
   case 110:stop(w);advance(w,10000);break;
   case 108:{stop(w);exe_loaded=false;entry=0;unsigned n=ps1::install_polygon_demo(*bus);cpu->reset(0);advance(w,n);status=L"Polygon demo: tam giac va tu giac mau phang qua CPU -> GP0.";refresh();break;}
   case 107:{stop(w);exe_loaded=false;entry=0;unsigned n=ps1::install_gpu_demo(*bus);cpu->reset(0);advance(w,n);status=L"GPU demo: CPU -> SW -> GP0 -> VRAM. Khung xem 320x240.";refresh();break;}
   case 102:loadBios(w);break;
   case 103:stop(w);cpu->reset(entry);if(exe_loaded)cpu->r=initial_regs;status=L"CPU reset; RAM duoc giu lai.";refresh();break;
   case 104:stop(w);advance(w,1);break;
   case 105:if(!running)start_run(w);break;
   case 106:stop(w);status=L"Da tam dung.";refresh();break;
  }return 0;
 case WM_TIMER:if(wp==1&&running){
  update_pad(w);sample_perf();
  if(!runner->active()){
   const auto error=runner->error();stop(w);status=L"Dung: "+std::wstring(error.begin(),error.end());last_fault=status;refresh();return 0;
  }
  if(audio_output.error()!=MMSYSERR_NOERROR&&audio_output.ready()==false){audio_output.close();status=L"Loi phat am thanh Windows; CPU van chay.";}
  bool changed=false;
  {std::lock_guard<std::mutex> lock(machine_mutex);changed=(bus->gpu.words!=painted_words&&bus->gpu.video_frames!=painted_video)||bus->gpu.display_revision!=painted_display;}
  if(changed&&!IsIconic(w))InvalidateRect(screen,nullptr,FALSE);
  if(GetTickCount64()-last_refresh>=250){refresh();last_refresh=GetTickCount64();}
 }return 0;
 case WM_DESTROY:stop(w);if(font)DeleteObject(font);PostQuitMessage(0);return 0;
 }return DefWindowProcW(w,m,wp,lp);
}
}
LRESULT CALLBACK vramProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 if(m==WM_LBUTTONDOWN){SetFocus(w);return 0;}
 if(m==WM_KEYDOWN&&wp==VK_ESCAPE){SendMessageW(GetParent(w),WM_COMMAND,106,0);return 0;}
 if(m==WM_PAINT){
  PAINTSTRUCT paint{};HDC dc=BeginPaint(w,&paint);
  if(show_welcome){
   RECT area{};GetClientRect(w,&area);FillRect(dc,&area,background_brush);
   if(welcome_art&&welcome_art->GetLastStatus()==Gdiplus::Ok){
    Gdiplus::Graphics graphics(dc);graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    const float iw=float(welcome_art->GetWidth()),ih=float(welcome_art->GetHeight());
    const float scale=std::max(float(area.right)/iw,float(area.bottom)/ih);
    graphics.DrawImage(welcome_art.get(),Gdiplus::RectF((area.right-iw*scale)/2,(area.bottom-ih*scale)/2,iw*scale,ih*scale));
   }
   ps1ui::label(dc,L"PRESS PLAY.",RECT{40,40,area.right-30,100},title_font,ps1ui::text);
   ps1ui::label(dc,L"A new chapter for your classics.",RECT{42,104,area.right-30,140},ui_font,ps1ui::muted);
   ps1ui::label(dc,L"01  Load BIOS     02  Open disc     03  Boot disc + Play",RECT{40,area.bottom-70,area.right-20,area.bottom-24},small_font,ps1ui::accent);
   EndPaint(w,&paint);return 0;
  }
  std::unique_lock<std::mutex> lock(machine_mutex);
  const auto view=ps1::display_view(bus->gpu);
  std::copy(bus->gpu.vram.begin(),bus->gpu.vram.end(),presentation.vram.begin());
  painted_words=bus->gpu.words;painted_display=bus->gpu.display_revision;painted_video=bus->gpu.video_frames;lock.unlock();
  presentation_pixels.resize(view.width*view.height);
  auto& pixels=presentation_pixels;
  for(unsigned y=0;y<view.height;y++)for(unsigned x=0;x<view.width;x++)pixels[y*view.width+x]=ps1::display_pixel(presentation,view,x,y);
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=static_cast<LONG>(view.width);info.bmiHeader.biHeight=-static_cast<LONG>(view.height);
  info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  RECT r{};GetClientRect(w,&r);SetStretchBltMode(dc,COLORONCOLOR);
  FillRect(dc,&r,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
  const auto dest=ps1::fit_display(r.right,r.bottom);
  if(dest.width>0&&dest.height>0)StretchDIBits(dc,dest.x,dest.y,dest.width,dest.height,0,0,static_cast<int>(view.width),static_cast<int>(view.height),pixels.data(),&info,DIB_RGB_COLORS,SRCCOPY);
  EndPaint(w,&paint);return 0;
 }
 return DefWindowProcW(w,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int show){
 Gdiplus::GdiplusStartupInput startup;ULONG_PTR gdiplus_token=0;
 if(Gdiplus::GdiplusStartup(&gdiplus_token,&startup,nullptr)!=Gdiplus::Ok)return 1;
 background_brush=CreateSolidBrush(ps1ui::background);panel_brush=CreateSolidBrush(ps1ui::panel);
 ui_font=CreateFontW(-16,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 title_font=CreateFontW(-34,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 small_font=CreateFontW(-13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,32768);
 auto art_path=std::filesystem::path(executable).parent_path()/L"assets"/L"welcome.png";
 if(!std::filesystem::exists(art_path))art_path=std::filesystem::path(L"assets")/L"welcome.png";
 welcome_art=std::make_unique<Gdiplus::Image>(art_path.c_str());
 bus=std::make_unique<ps1::Bus>();cpu=std::make_unique<ps1::Cpu>(*bus);ps1::install_demo(*bus);cpu->reset(0);
 WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=h;c.lpszClassName=L"PS1LabWindow";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=background_brush;
 WNDCLASSW v{};v.lpfnWndProc=vramProc;v.hInstance=h;v.lpszClassName=L"PS1Vram";v.hCursor=LoadCursor(nullptr,IDC_ARROW);
 if(!RegisterClassW(&v))return 1;
 if(!RegisterClassW(&c))return 1;
 HWND w=CreateWindowW(c.lpszClassName,L"PSLab v30.1 - PS2 Hardware and GIF DMA",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,h,nullptr);
 if(!w)return 1;ShowWindow(w,show);MSG msg{};int result;
 while((result=GetMessageW(&msg,nullptr,0,0))>0){
  if((msg.message==WM_KEYDOWN||msg.message==WM_KEYUP)&&GetAncestor(msg.hwnd,GA_ROOT)==w){
   if(keyboard.event(unsigned(msg.wParam),msg.message==WM_KEYDOWN)){update_pad(w);continue;}
  }
  TranslateMessage(&msg);DispatchMessageW(&msg);
 }
 welcome_art.reset();DeleteObject(ui_font);DeleteObject(title_font);DeleteObject(small_font);DeleteObject(background_brush);DeleteObject(panel_brush);Gdiplus::GdiplusShutdown(gdiplus_token);
 return result==-1?1:static_cast<int>(msg.wParam);
}
