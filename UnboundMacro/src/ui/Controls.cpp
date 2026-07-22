#include "Controls.h"

#include "Theme.h"

#include <commctrl.h>

bool StyledButton::Create(
    HWND parent,
    int id,
    const RECT& rect,
    const std::wstring& text,
    HFONT font)
{
    text_ = text;
    font_ = font;
    style_.face = Theme::Colors().button_face;
    style_.face_hover = Theme::Colors().button_hover;
    style_.text = Theme::Colors().text;
    style_.border = Theme::Colors().border;

    hwnd_ = CreateWindowExW(
        0,
        L"BUTTON",
        text.c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr),
        nullptr);

    if (!hwnd_) {
        return false;
    }

    SetWindowSubclass(hwnd_, SubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    return true;
}

void StyledButton::SetText(const std::wstring& text)
{
    text_ = text;
    if (hwnd_) {
        SetWindowTextW(hwnd_, text.c_str());
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void StyledButton::SetStyle(const ButtonStyle& style)
{
    style_ = style;
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

HWND StyledButton::Handle() const
{
    return hwnd_;
}

void StyledButton::Paint(HDC hdc) const
{
    RECT rc = {};
    GetClientRect(hwnd_, &rc);

    const COLORREF fill = hovered_ ? style_.face_hover : style_.face;
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, style_.border);
    HGDIOBJ old_brush = SelectObject(hdc, brush);
    HGDIOBJ old_pen = SelectObject(hdc, pen);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, style_.text);
    HGDIOBJ old_font = nullptr;
    if (font_) {
        old_font = SelectObject(hdc, font_);
    }
    DrawTextW(
        hdc,
        text_.c_str(),
        -1,
        &rc,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (old_font) {
        SelectObject(hdc, old_font);
    }
}

LRESULT CALLBACK StyledButton::SubclassProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam,
    UINT_PTR,
    DWORD_PTR ref_data)
{
    auto* self = reinterpret_cast<StyledButton*>(ref_data);

    switch (msg) {
    case WM_MOUSEMOVE: {
        if (!self->hovered_) {
            self->hovered_ = true;
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    }
    case WM_MOUSELEAVE:
        self->hovered_ = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    case WM_SETTEXT:
        if (self) {
            self->text_ = reinterpret_cast<const wchar_t*>(lparam);
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return TRUE;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps = {};
        HDC hdc = BeginPaint(hwnd, &ps);
        self->Paint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, SubclassProc, 1);
        break;
    default:
        break;
    }

    return DefSubclassProc(hwnd, msg, wparam, lparam);
}
