#pragma once

#include <windows.h>

struct ThemeColors {
    COLORREF background;
    COLORREF panel;
    COLORREF border;
    COLORREF text;
    COLORREF muted;
    COLORREF button_face;
    COLORREF button_hover;
    COLORREF success;
};

namespace Theme {
const ThemeColors& Colors();
HBRUSH BackgroundBrush();
void Shutdown();
}
