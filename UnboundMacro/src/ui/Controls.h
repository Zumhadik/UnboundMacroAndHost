#pragma once

#include <windows.h>
#include <string>

struct ButtonStyle {
    COLORREF face;
    COLORREF face_hover;
    COLORREF text;
    COLORREF border;
};

class StyledButton {
public:
    bool Create(
        HWND parent,
        int id,
        const RECT& rect,
        const std::wstring& text,
        HFONT font);
    void SetText(const std::wstring& text);
    void SetStyle(const ButtonStyle& style);
    HWND Handle() const;

    static LRESULT CALLBACK SubclassProc(
        HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam,
        UINT_PTR subclass_id,
        DWORD_PTR ref_data);

private:
    void Paint(HDC hdc) const;

    HWND hwnd_ = nullptr;
    std::wstring text_;
    ButtonStyle style_{};
    bool hovered_ = false;
    HFONT font_ = nullptr;
};
