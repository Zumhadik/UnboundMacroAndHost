#include "rdphost/RegistryConfig.h"

#include "rdphost/WinUtils.h"

#include <windows.h>

#include <cwctype>
#include <vector>

namespace rdphost {
namespace {

constexpr wchar_t kTermServerKey[] =
    L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server";
constexpr wchar_t kTermServiceParamsKey[] =
    L"SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters";
constexpr wchar_t kLicensingCoreKey[] =
    L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Licensing Core";
constexpr wchar_t kWinlogonKey[] =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

bool SetDword(HKEY root, const wchar_t* subkey, const wchar_t* name, DWORD value) {
    HKEY key = nullptr;
    const LONG open = RegCreateKeyExW(
        root,
        subkey,
        0,
        nullptr,
        0,
        KEY_SET_VALUE | KEY_WOW64_64KEY,
        nullptr,
        &key,
        nullptr);
    if (open != ERROR_SUCCESS) {
        SetLastError(static_cast<DWORD>(open));
        return false;
    }

    const LONG write = RegSetValueExW(
        key,
        name,
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&value),
        sizeof(value));
    RegCloseKey(key);
    if (write != ERROR_SUCCESS) {
        SetLastError(static_cast<DWORD>(write));
        return false;
    }
    return true;
}
bool SetExpandString(HKEY root,
                     const wchar_t* subkey,
                     const wchar_t* name,
                     const std::wstring& value) {
    HKEY key = nullptr;
    const LONG open = RegCreateKeyExW(
        root,
        subkey,
        0,
        nullptr,
        0,
        KEY_SET_VALUE | KEY_WOW64_64KEY,
        nullptr,
        &key,
        nullptr);
    if (open != ERROR_SUCCESS) {
        SetLastError(static_cast<DWORD>(open));
        return false;
    }

    const DWORD bytes =
        static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const LONG write = RegSetValueExW(
        key,
        name,
        0,
        REG_EXPAND_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        bytes);
    RegCloseKey(key);
    if (write != ERROR_SUCCESS) {
        SetLastError(static_cast<DWORD>(write));
        return false;
    }
    return true;
}
bool SetString(HKEY root,
               const wchar_t* subkey,
               const wchar_t* name,
               const std::wstring& value) {
    HKEY key = nullptr;
    const LONG open = RegCreateKeyExW(
        root,
        subkey,
        0,
        nullptr,
        0,
        KEY_SET_VALUE | KEY_WOW64_64KEY,
        nullptr,
        &key,
        nullptr);
    if (open != ERROR_SUCCESS) {
        SetLastError(static_cast<DWORD>(open));
        return false;
    }

    const DWORD bytes =
        static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const LONG write = RegSetValueExW(
        key,
        name,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        bytes);
    RegCloseKey(key);
    if (write != ERROR_SUCCESS) {
        SetLastError(static_cast<DWORD>(write));
        return false;
    }
    return true;
}
bool ReadDword(HKEY root,
               const wchar_t* subkey,
               const wchar_t* name,
               DWORD* value) {
    HKEY key = nullptr;
    const LONG open = RegOpenKeyExW(
        root,
        subkey,
        0,
        KEY_QUERY_VALUE | KEY_WOW64_64KEY,
        &key);
    if (open != ERROR_SUCCESS) {
        SetLastError(static_cast<DWORD>(open));
        return false;
    }

    DWORD type = 0;
    DWORD size = sizeof(DWORD);
    const LONG read = RegQueryValueExW(
        key,
        name,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(value),
        &size);
    RegCloseKey(key);
    if (read != ERROR_SUCCESS || type != REG_DWORD) {
        SetLastError(static_cast<DWORD>(read == ERROR_SUCCESS ? ERROR_INVALID_DATA : read));
        return false;
    }
    return true;
}
std::wstring ReadStringValue(HKEY root, const wchar_t* subkey, const wchar_t* name) {
    HKEY key = nullptr;
    const LONG open = RegOpenKeyExW(
        root,
        subkey,
        0,
        KEY_QUERY_VALUE | KEY_WOW64_64KEY,
        &key);
    if (open != ERROR_SUCCESS) {
        return {};
    }

    DWORD type = 0;
    DWORD size = 0;
    LONG query = RegQueryValueExW(key, name, nullptr, &type, nullptr, &size);
    if (query != ERROR_SUCCESS || size == 0) {
        RegCloseKey(key);
        return {};
    }

    std::vector<wchar_t> buffer(size / sizeof(wchar_t) + 1);
    query = RegQueryValueExW(
        key,
        name,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(buffer.data()),
        &size);
    RegCloseKey(key);
    if (query != ERROR_SUCCESS) {
        return {};
    }

    return buffer.data();
}
bool ContainsInsensitive(const std::wstring& text, const wchar_t* needle) {
    std::wstring lower = text;
    std::wstring target = needle;
    for (wchar_t& ch : lower) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    for (wchar_t& ch : target) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    return lower.find(target) != std::wstring::npos;
}
}
StatusItem RegistryConfig::CheckServiceDll() const {
    StatusItem item;
    item.id = L"service_dll";
    item.title = L"ServiceDll";

    const std::wstring value = ReadServiceDll();
    if (value.empty()) {
        item.level = StatusLevel::Error;
        item.detail = L"ServiceDll is empty / unreadable";
        return item;
    }

    item.detail = value;
    item.level = ContainsInsensitive(value, L"rdpwrap.dll")
                     ? StatusLevel::Ok
                     : StatusLevel::Error;
    return item;
}
OperationResult RegistryConfig::ApplyInstallKeys(
    const std::wstring& serviceDllExpandPath) {
    if (!SetExpandString(
            HKEY_LOCAL_MACHINE,
            kTermServiceParamsKey,
            L"ServiceDll",
            serviceDllExpandPath)) {
        return WinUtils::LastErrorAsResult(L"Set ServiceDll");
    }

    if (!SetDword(HKEY_LOCAL_MACHINE, kTermServerKey, L"fDenyTSConnections", 0)) {
        return WinUtils::LastErrorAsResult(L"Set fDenyTSConnections");
    }

    if (!SetDword(
            HKEY_LOCAL_MACHINE,
            kLicensingCoreKey,
            L"EnableConcurrentSessions",
            1)) {
        return WinUtils::LastErrorAsResult(L"Set EnableConcurrentSessions");
    }

    if (!SetDword(
            HKEY_LOCAL_MACHINE,
            kWinlogonKey,
            L"AllowMultipleTSSessions",
            1)) {
        return WinUtils::LastErrorAsResult(L"Set AllowMultipleTSSessions");
    }

    SetString(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\Clip Redirector",
        L"Name",
        L"RDPClip");
    SetDword(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\Clip Redirector",
        L"Type",
        3);
    SetString(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\DND Redirector",
        L"Name",
        L"RDPDND");
    SetDword(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\DND Redirector",
        L"Type",
        3);
    SetDword(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\AddIns\\Dynamic VC",
        L"Type",
        0xFFFFFFFF);

    return {true, L"Registry install keys applied"};
}
std::wstring RegistryConfig::ReadServiceDll() const {
    return ReadStringValue(HKEY_LOCAL_MACHINE, kTermServiceParamsKey, L"ServiceDll");
}
}