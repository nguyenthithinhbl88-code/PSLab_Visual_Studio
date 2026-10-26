#pragma once
#include <windows.h>
namespace ps1ui {
constexpr COLORREF background=RGB(12,17,23),panel=RGB(19,27,35),border=RGB(39,53,64),text=RGB(226,236,240),muted=RGB(137,157,170),accent=RGB(86,225,196);
inline void box(HDC dc,RECT r,COLORREF fill,COLORREF line,int radius=14){
 auto brush=CreateSolidBrush(fill);auto pen=CreatePen(PS_SOLID,1,line);auto oldb=SelectObject(dc,brush),oldp=SelectObject(dc,pen);
 RoundRect(dc,r.left,r.top,r.right,r.bottom,radius,radius);SelectObject(dc,oldb);SelectObject(dc,oldp);DeleteObject(brush);DeleteObject(pen);
}
inline void label(HDC dc,const wchar_t* value,RECT r,HFONT font,COLORREF color,UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE){
 auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);DrawTextW(dc,value,-1,&r,flags);SelectObject(dc,old);
}
}
