#pragma once

#include "rdphost/Types.h"

#include <windows.h>

#include <string>

namespace rdphost {

class WinUtils {
public:
    static bool IsProcessElevated();
    static std::wstring ExpandEnv(const std::wstring& value);
    static OperationResult LastErrorAsResult(const std::wstring& prefix);
    static bool CopyTextToClipboard(HWND owner, const std::wstring& text);
};

}