#include "StatusPanel.h"

#include "Theme.h"

std::wstring StatusPanel::FormatTime(std::uint64_t elapsed_ms)
{
    const unsigned total_sec = static_cast<unsigned>(elapsed_ms / 1000);
    const unsigned minutes = (total_sec / 60) % 100;
    const unsigned seconds = total_sec % 60;

    wchar_t buffer[16] = {};
    swprintf_s(buffer, L"%02u:%02u", minutes, seconds);
    return buffer;
}

std::wstring StatusPanel::StatusText(const AppState& state)
{
    if (state.waiting_keybind) {
        return L"Waiting keybind...";
    }
    if (!state.has_file) {
        return L"No file";
    }
    if (state.is_recording) {
        return L"Recording";
    }
    if (state.is_playing) {
        return L"Playing";
    }
    return L"Ready";
}

void StatusPanel::SetFont(HFONT font)
{
    font_ = font;
}

void StatusPanel::Paint(HDC hdc, const RECT& area, const AppState& state) const
{
    const ThemeColors& colors = Theme::Colors();

    HBRUSH brush = CreateSolidBrush(colors.panel);
    HPEN pen = CreatePen(PS_SOLID, 1, colors.border);
    HGDIOBJ old_brush = SelectObject(hdc, brush);
    HGDIOBJ old_pen = SelectObject(hdc, pen);
    RoundRect(hdc, area.left, area.top, area.right, area.bottom, 8, 8);
    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);

    SetBkMode(hdc, TRANSPARENT);
    HGDIOBJ old_font = nullptr;
    if (font_) {
        old_font = SelectObject(hdc, font_);
    }

    RECT line = area;
    line.left += 14;
    line.right -= 14;
    line.top += 10;
    line.bottom = line.top + 22;

    SetTextColor(hdc, colors.muted);
    DrawTextW(hdc, L"Status:", -1, &line, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT value = line;
    value.left += 70;
    SetTextColor(hdc, colors.text);
    const std::wstring status = StatusText(state);
    DrawTextW(
        hdc,
        status.c_str(),
        -1,
        &value,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    OffsetRect(&line, 0, 24);
    OffsetRect(&value, 0, 24);
    SetTextColor(hdc, colors.muted);
    DrawTextW(hdc, L"Time:", -1, &line, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, colors.text);
    std::wstring time_text;
    if (!state.has_file) {
        time_text = L"-";
    } else {
        time_text = FormatTime(state.elapsed_ms);
    }
    DrawTextW(
        hdc,
        time_text.c_str(),
        -1,
        &value,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    OffsetRect(&line, 0, 24);
    OffsetRect(&value, 0, 24);
    SetTextColor(hdc, colors.muted);
    DrawTextW(hdc, L"Loop:", -1, &line, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(
        hdc,
        state.loop_enabled ? colors.success : colors.text);
    DrawTextW(
        hdc,
        state.loop_enabled ? L"On" : L"Off",
        -1,
        &value,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    if (old_font) {
        SelectObject(hdc, old_font);
    }
}
