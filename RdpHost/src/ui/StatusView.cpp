#include "rdphost/ui/StatusView.h"

#include "rdphost/ui/UiColors.h"

#include <string>

namespace rdphost::ui {
namespace {

constexpr wchar_t kClassName[] = L"RdpHostStatusView";

}
bool StatusView::class_registered_ = false;

StatusView::~StatusView() {
    if (hwnd_ != nullptr) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}
bool StatusView::Create(HWND parent, int x, int y, int width, int height) {
    if (!class_registered_) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = StatusView::WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = kClassName;
        if (RegisterClassExW(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
        class_registered_ = true;
    }

    hwnd_ = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        kClassName,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        x,
        y,
        width,
        height,
        parent,
        nullptr,
        GetModuleHandleW(nullptr),
        this);

    return hwnd_ != nullptr;
}
void StatusView::SetItems(const StatusList& items) {
    items_ = items;
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}
std::wstring StatusView::FormatText() const {
    std::wstring text;
    for (const StatusItem& item : items_) {
        text += L"[";
        text += TextForStatus(item.level);
        text += L"] ";
        text += item.title;
        text += L" - ";
        text += item.detail;
        text += L"\r\n";
    }
    return text;
}
HWND StatusView::Handle() const {
    return hwnd_;
}
LRESULT CALLBACK StatusView::WndProc(HWND hwnd,
                                     UINT msg,
                                     WPARAM wparam,
                                     LPARAM lparam) {
    StatusView* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<StatusView*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<StatusView*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self != nullptr) {
        return self->HandleMessage(msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}
LRESULT StatusView::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd_, &ps);
            Paint(hdc);
            EndPaint(hwnd_, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        default:
            break;
    }
    return DefWindowProcW(hwnd_, msg, wparam, lparam);
}
void StatusView::Paint(HDC hdc) {
    RECT client{};
    GetClientRect(hwnd_, &client);
    FillRect(hdc, &client, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

    HFONT font = CreateFontW(
        -14,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");
    HFONT old = static_cast<HFONT>(SelectObject(hdc, font));

    int y = 8;
    for (const StatusItem& item : items_) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, ColorForStatus(item.level));

        std::wstring line = L"[";
        line += TextForStatus(item.level);
        line += L"] ";
        line += item.title;
        line += L" - ";
        line += item.detail;

        RECT row{8, y, client.right - 8, y + 22};
        DrawTextW(hdc, line.c_str(), -1, &row, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        y += 24;
    }

    SelectObject(hdc, old);
    DeleteObject(font);
}
}