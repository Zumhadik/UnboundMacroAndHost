#pragma once

#include "../core/Types.h"

#include <windows.h>
#include <string>

class StatusPanel {
public:
    void SetFont(HFONT font);
    void Paint(HDC hdc, const RECT& area, const AppState& state) const;

private:
    static std::wstring FormatTime(std::uint64_t elapsed_ms);
    static std::wstring StatusText(const AppState& state);

    HFONT font_ = nullptr;
};
