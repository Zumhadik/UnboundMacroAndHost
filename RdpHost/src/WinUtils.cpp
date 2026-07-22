#include "rdphost/WinUtils.h"

#include <windows.h>

#include <cstring>
#include <sstream>

namespace rdphost {
namespace {

std::wstring FormatWinError(DWORD code) {
    wchar_t* buffer = nullptr;
    const DWORD flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = FormatMessageW(
        flags,
        nullptr,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);

    std::wstring text;
    if (length != 0 && buffer != nullptr) {
        text.assign(buffer, length);
        while (!text.empty() &&
               (text.back() == L'\r' || text.back() == L'\n' ||
                text.back() == L' ')) {
            text.pop_back();
        }
        LocalFree(buffer);
    } else {
        text = L"Unknown error";
    }

    std::wstringstream stream;
    stream << text << L" (" << code << L")";
    return stream.str();
}
}
bool WinUtils::IsProcessElevated() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }

    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    const BOOL ok = GetTokenInformation(
        token,
        TokenElevation,
        &elevation,
        sizeof(elevation),
        &size);
    CloseHandle(token);
    return ok == TRUE && elevation.TokenIsElevated != 0;
}
std::wstring WinUtils::ExpandEnv(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const DWORD needed =
        ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
    if (needed == 0) {
        return value;
    }

    std::wstring result(needed, L'\0');
    const DWORD written =
        ExpandEnvironmentStringsW(value.c_str(), result.data(), needed);
    if (written == 0) {
        return value;
    }

    if (!result.empty() && result.back() == L'\0') {
        result.pop_back();
    }
    return result;
}
OperationResult WinUtils::LastErrorAsResult(const std::wstring& prefix) {
    const DWORD code = GetLastError();
    OperationResult result;
    result.ok = false;
    result.message = prefix + L": " + FormatWinError(code);
    return result;
}
bool WinUtils::CopyTextToClipboard(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) {
        return false;
    }

    const bool emptied = EmptyClipboard() != FALSE;
    if (!emptied) {
        CloseClipboard();
        return false;
    }

    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (mem == nullptr) {
        CloseClipboard();
        return false;
    }

    void* locked = GlobalLock(mem);
    if (locked == nullptr) {
        GlobalFree(mem);
        CloseClipboard();
        return false;
    }

    memcpy(locked, text.c_str(), bytes);
    GlobalUnlock(mem);

    if (SetClipboardData(CF_UNICODETEXT, mem) == nullptr) {
        GlobalFree(mem);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}
}