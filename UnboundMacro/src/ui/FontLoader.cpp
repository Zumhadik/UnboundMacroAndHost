#include "FontLoader.h"

#include "resource.h"

bool FontLoader::LoadFromResource(HINSTANCE instance, int resource_id)
{
    Unload();

    const HRSRC resource = FindResourceW(
        instance,
        MAKEINTRESOURCEW(resource_id),
        RT_RCDATA);
    if (!resource) {
        return false;
    }

    const HGLOBAL data = LoadResource(instance, resource);
    if (!data) {
        return false;
    }

    const void* bytes = LockResource(data);
    const DWORD size = SizeofResource(instance, resource);
    if (!bytes || size == 0) {
        return false;
    }

    DWORD installed = 0;
    font_handle_ = AddFontMemResourceEx(
        const_cast<void*>(bytes),
        size,
        nullptr,
        &installed);
    return font_handle_ != nullptr && installed > 0;
}

HFONT FontLoader::CreateUiFont(int height_px) const
{
    const wchar_t* names[] = {
        L"SF Pro Display",
        L"SFProDisplay",
        L"SF Pro Display Bold",
        L"SFProDisplayBold"
    };

    for (const wchar_t* name : names) {
        HFONT font = CreateFontW(
            -height_px,
            0,
            0,
            0,
            FW_BOLD,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            name);
        if (font) {
            return font;
        }
    }

    return nullptr;
}

void FontLoader::Unload()
{
    if (font_handle_) {
        RemoveFontMemResourceEx(font_handle_);
        font_handle_ = nullptr;
    }
}
