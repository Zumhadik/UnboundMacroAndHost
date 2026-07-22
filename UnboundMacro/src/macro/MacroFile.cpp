#include "MacroFile.h"

#include "MacroFormat.h"
#include "../core/Constants.h"

#include <commdlg.h>

namespace {
bool PickPath(bool save_mode, std::wstring* out_path)
{
    wchar_t buffer[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = MACRO_FILTER;
    ofn.lpstrDefExt = MACRO_EXT;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;

    if (save_mode) {
        ofn.Flags |= OFN_OVERWRITEPROMPT;
        if (!GetSaveFileNameW(&ofn)) {
            return false;
        }
    } else {
        ofn.Flags |= OFN_FILEMUSTEXIST;
        if (!GetOpenFileNameW(&ofn)) {
            return false;
        }
    }

    *out_path = buffer;
    return true;
}
}

bool MacroFile::CreateNew(std::wstring* out_path)
{
    if (!PickPath(true, out_path)) {
        return false;
    }
    return MacroFormat::WriteEmpty(*out_path);
}

bool MacroFile::OpenExisting(std::wstring* out_path)
{
    return PickPath(false, out_path);
}
