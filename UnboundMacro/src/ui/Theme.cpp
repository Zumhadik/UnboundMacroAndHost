#include "Theme.h"

namespace {
ThemeColors g_colors = {
    RGB(22, 24, 28),
    RGB(32, 35, 41),
    RGB(52, 56, 64),
    RGB(220, 224, 230),
    RGB(140, 148, 160),
    RGB(40, 44, 52),
    RGB(50, 56, 66),
    RGB(100, 160, 120),
};

HBRUSH g_bg_brush = nullptr;
}

const ThemeColors& Theme::Colors()
{
    return g_colors;
}

HBRUSH Theme::BackgroundBrush()
{
    if (!g_bg_brush) {
        g_bg_brush = CreateSolidBrush(g_colors.background);
    }
    return g_bg_brush;
}

void Theme::Shutdown()
{
    if (g_bg_brush) {
        DeleteObject(g_bg_brush);
        g_bg_brush = nullptr;
    }
}
