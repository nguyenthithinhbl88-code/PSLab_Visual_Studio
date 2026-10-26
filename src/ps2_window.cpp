#include "ps2_window.hpp"
#include "ps2_ee.hpp"
#include <commdlg.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <chrono>
namespace {
using Clock=std::chrono::steady_clock;
HWND current=nullptr;
struct State{
 ps2::Ee ee;HWND output=nullptr,screen=nullptr,run_button=nullptr;
 bool running=false;Clock::time_point refreshed=Clock::now();
 std::wstring message=L"Experimental PS2. GPU demo draws through CPU/GIF. BIOS boot remains incomplete.";
};
void pause(State& s){s.running=false;SetWindowTextW(s.run_button,L"Run continuously");}
void refresh(State& s){
 auto d=s.ee.diagnostic();SetWindowTextW(s.output,(s.message+L"\r\n\r\n"+std::wstring(d.begin(),d.end())).c_str());
 if(s.screen)InvalidateRect(s.screen,nullptr,FALSE);
 s.refreshed=Clock::now();
}
LRESULT CALLBACK video_proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 auto s=reinterpret_cast<State*>(GetWindowLongPtrW(w,GWLP_USERDATA));
 if(m==WM_NCCREATE){s=static_cast<State*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));}
 if(m==WM_CLOSE){ShowWindow(w,SW_HIDE);return 0;}
 if(m==WM_SIZE){InvalidateRect(w,nullptr,FALSE);return 0;}
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT&&s){
  PAINTSTRUCT paint{};HDC dc=BeginPaint(w,&paint);RECT r{};GetClientRect(w,&r);FillRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
  SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(130,215,240));
  try{
   auto image=s->ee.gs.scanout();int available_w=std::max(1L,r.right-24),available_h=std::max(1L,r.bottom-70);
   double scale=std::min(double(available_w)/image.width,double(available_h)/image.height);
   int width=std::max(1,int(image.width*scale)),height=std::max(1,int(image.height*scale));
   int x=(r.right-width)/2,y=44+(available_h-height)/2;
   BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=LONG(image.width);info.bmiHeader.biHeight=-LONG(image.height);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
   SetStretchBltMode(dc,COLORONCOLOR);StretchDIBits(dc,x,y,width,height,0,0,int(image.width),int(image.height),image.bgra.data(),&info,DIB_RGB_COLORS,SRCCOPY);
   std::wstring caption=L"PS2 GS | basic software renderer | "+std::to_wstring(image.width)+L" x "+std::to_wstring(image.height);
   if(!image.enabled)caption+=L" | display circuit disabled";else if(!image.supported)caption+=L" | framebuffer format unsupported";
   TextOutW(dc,12,12,caption.c_str(),int(caption.size()));
   const wchar_t* note=L"Untextured rendering only. This screen does not confirm BIOS/game compatibility.";
   TextOutW(dc,12,std::max(44L,r.bottom-22),note,lstrlenW(note));
  }catch(const std::exception&){const wchar_t* error=L"GS scanout unavailable";TextOutW(dc,12,12,error,lstrlenW(error));}
  EndPaint(w,&paint);return 0;
 }
 if(m==WM_NCDESTROY){if(s&&s->screen==w)s->screen=nullptr;SetWindowLongPtrW(w,GWLP_USERDATA,0);}
 return DefWindowProcW(w,m,wp,lp);
}
void show_screen(HWND owner,State& s){
 if(!s.screen){auto instance=GetModuleHandleW(nullptr);WNDCLASSW c{};c.lpfnWndProc=video_proc;c.hInstance=instance;c.lpszClassName=L"PSLabPs2GsScreen";c.hCursor=LoadCursor(nullptr,IDC_ARROW);WNDCLASSW existing{};if(!GetClassInfoW(instance,c.lpszClassName,&existing)&&!RegisterClassW(&c))throw std::runtime_error("Cannot register GS screen");
  s.screen=CreateWindowW(c.lpszClassName,L"PSLab v30.1 - PS2 GS Screen",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,900,720,owner,nullptr,instance,&s);if(!s.screen)throw std::runtime_error("Cannot create GS screen");
 }
 ShowWindow(s.screen,SW_SHOW);InvalidateRect(s.screen,nullptr,FALSE);
}
std::vector<uint8_t> load(HWND w,bool bios){wchar_t path[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=path;o.nMaxFile=32768;o.lpstrFilter=bios?L"PS2 BIOS\0*.bin\0All\0*.*\0":L"PS2 homebrew ELF\0*.elf\0All\0*.*\0";o.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;if(!GetOpenFileNameW(&o))return {};std::ifstream f(std::filesystem::path(path),std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||f.tellg()>64*1024*1024)throw std::runtime_error("Input file unavailable or exceeds 64 MiB");std::vector<uint8_t> bytes(static_cast<size_t>(f.tellg()));f.seekg(0);if(!f.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())))throw std::runtime_error("Read failed");return bytes;}
void save(HWND w,State& s){wchar_t path[32768]=L"pslab_ps2_log.txt";OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=w;o.lpstrFile=path;o.nMaxFile=32768;o.lpstrFilter=L"Text log\0*.txt\0All\0*.*\0";o.lpstrDefExt=L"txt";o.Flags=OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR;if(GetSaveFileNameW(&o)){std::ofstream f(std::filesystem::path(path),std::ios::binary);f<<s.ee.diagnostic();if(!f)throw std::runtime_error("Cannot save log");s.message=L"Diagnostic log saved.";}}
LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){auto s=reinterpret_cast<State*>(GetWindowLongPtrW(w,GWLP_USERDATA));
 if(m==WM_CREATE){
  auto owned=std::make_unique<State>();s=owned.get();SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));owned.release();
  const wchar_t* labels[]={L"Original demo",L"Open ELF",L"Step",L"Run 1k",L"Reset PC",L"Load BIOS (test)",L"CPU + GS demo",L"GIF demo",L"Run 100k",L"Save log",L"Run continuously",L"Stop",L"GS Screen",L"GPU demo",L"DMA GPU demo"};
  for(int i=0;i<15;i++){auto button=CreateWindowW(L"BUTTON",labels[i],WS_VISIBLE|WS_CHILD|WS_TABSTOP,12+(i%5)*190,12+(i/5)*38,184,32,w,reinterpret_cast<HMENU>(INT_PTR(501+i)),nullptr,nullptr);if(i==10)s->run_button=button;}
  s->output=CreateWindowW(L"EDIT",L"",WS_VISIBLE|WS_CHILD|WS_VSCROLL|ES_MULTILINE|ES_READONLY,12,130,940,600,w,nullptr,nullptr,nullptr);
  SendMessageW(s->output,EM_SETLIMITTEXT,1024*1024,0);SendMessageW(s->output,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(ANSI_FIXED_FONT)),TRUE);
  s->ee.demo();refresh(*s);if(!SetTimer(w,1,16,nullptr)){s->message=L"Cannot start run scheduler.";refresh(*s);}return 0;
 }
 if(m==WM_SIZE&&s){RECT r{};GetClientRect(w,&r);MoveWindow(s->output,12,130,std::max(1L,r.right-24),std::max(1L,r.bottom-142),TRUE);return 0;}
 if(m==WM_TIMER&&wp==1&&s&&s->running){
  try{
   auto until=Clock::now()+std::chrono::milliseconds(4);
   for(unsigned n=0;n<10000;n++){s->ee.step();if((n&63)==63&&Clock::now()>=until)break;}
   if(s->screen&&IsWindowVisible(s->screen))InvalidateRect(s->screen,nullptr,FALSE);
   if(Clock::now()-s->refreshed>=std::chrono::milliseconds(100))refresh(*s);
  }catch(const std::exception& e){pause(*s);std::string error=e.what();s->message=L"Stopped automatically: "+std::wstring(error.begin(),error.end())+L". Save log, or Reset PC to retry.";refresh(*s);}
  return 0;
 }
 if(m==WM_COMMAND&&s){
  unsigned command=LOWORD(wp);if(command<501||command>515)return 0;
  bool was_running=s->running;pause(*s);
  try{switch(command){
   case 501:s->ee.demo();s->message=L"Original EE demo loaded. Run -> RAM[200]=c.";break;
   case 502:{auto b=load(w,false);if(!b.empty()){s->ee.load_elf(b);s->message=L"ELF loaded. Bare-metal supported subset only.";}break;}
   case 503:s->ee.step();s->message=L"One instruction executed.";break;
   case 504:for(unsigned n=0;n<1000;n++)s->ee.step();s->message=L"1000 EE steps executed.";break;
   case 505:s->ee.reset(s->ee.reset_entry);s->message=L"Reset to loaded entry; RAM retained. Press Run to start.";break;
   case 506:{auto b=load(w,true);if(!b.empty()){s->ee.load_bios(b);show_screen(w,*s);s->message=L"BIOS mapped. Press Run continuously. Complete PS2 BIOS boot not supported yet.";}break;}
   case 507:s->ee.register_demo();s->message=L"CPU/GS register demo loaded.";break;
   case 508:s->ee.gif_demo();s->message=L"GIF transport demo loaded; FRAME_1=10001 after Run.";break;
   case 509:for(unsigned n=0;n<100000;n++)s->ee.step();s->message=L"100000 EE steps executed.";break;
   case 510:save(w,*s);break;
   case 511:if(was_running){s->message=L"Paused. PC retained; Run resumes.";}else{if(!s->ee.first_fault.empty())throw std::runtime_error("Reset/load required after the recorded first fault");if(!SetTimer(w,1,16,nullptr))throw std::runtime_error("Cannot start run scheduler");s->running=true;SetWindowTextW(s->run_button,L"Pause");s->message=L"Running continuously. Pause or Stop at any time; errors stop automatically.";}break;
   case 512:s->message=L"Stopped. PC retained; Reset PC restarts the loaded program.";break;
   case 513:show_screen(w,*s);s->message=L"GS framebuffer screen opened.";break;
   case 515:s->ee.dma_demo();show_screen(w,*s);s->message=L"DMA GPU demo loaded. Run: RAM packets -> GIF DMA -> GS, 6 primitives. Normal mode only.";break;
   case 514:s->ee.gpu_demo();show_screen(w,*s);s->message=L"GPU demo loaded. Press Run continuously to draw shapes through CPU/GIF.";break;
  }}catch(const std::exception& e){pause(*s);std::string error=e.what();s->message=L"Stopped: "+std::wstring(error.begin(),error.end());}
  refresh(*s);return 0;
 }
 if(m==WM_DESTROY&&s){KillTimer(w,1);pause(*s);if(s->screen)DestroyWindow(s->screen);return 0;}
 if(m==WM_NCDESTROY){SetWindowLongPtrW(w,GWLP_USERDATA,0);delete s;current=nullptr;}
 return DefWindowProcW(w,m,wp,lp);
}
}
void open_ps2_window(HWND owner){if(current){ShowWindow(current,SW_RESTORE);SetForegroundWindow(current);return;}auto instance=GetModuleHandleW(nullptr);WNDCLASSW c{};c.lpfnWndProc=proc;c.hInstance=instance;c.lpszClassName=L"PSLabPs2Experimental";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);WNDCLASSW existing{};if(!GetClassInfoW(instance,c.lpszClassName,&existing)&&!RegisterClassW(&c))return;current=CreateWindowW(c.lpszClassName,L"PSLab v30.1 - PS2 Run / GS / BIOS laboratory",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1040,850,owner,nullptr,instance,nullptr);if(current)ShowWindow(current,SW_SHOW);}
