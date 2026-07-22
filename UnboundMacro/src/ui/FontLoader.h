#pragma once

#include <windows.h>

class FontLoader {
public:
    bool LoadFromResource(HINSTANCE instance, int resource_id);
    HFONT CreateUiFont(int height_px) const;
    void Unload();

private:
    HANDLE font_handle_ = nullptr;
};
