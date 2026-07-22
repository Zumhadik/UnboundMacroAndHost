#include "rdphost/IniSupport.h"

#include "rdphost/WinUtils.h"

#include <windows.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "version.lib")

namespace rdphost {
namespace {

std::wstring Join(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) {
        return right;
    }
    std::wstring out = left;
    if (out.back() != L'\\') {
        out.push_back(L'\\');
    }
    out += right;
    return out;
}
bool FileExists(const std::wstring& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES &&
           (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
bool FileContainsSection(const std::wstring& path, const std::wstring& section) {
    std::ifstream file{std::filesystem::path(path)};
    if (!file) {
        return false;
    }

    std::string needle = "[";
    for (const wchar_t ch : section) {
        if (ch > 0x7F) {
            return false;
        }
        needle.push_back(static_cast<char>(ch));
    }
    needle.push_back(']');

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == needle) {
            return true;
        }
    }
    return false;
}
std::wstring TermsrvPath() {
    return Join(WinUtils::ExpandEnv(L"%SystemRoot%\\System32"), L"termsrv.dll");
}
std::wstring FormatVersion(DWORD major, DWORD minor, DWORD build, DWORD revision) {
    std::wstringstream stream;
    stream << major << L'.' << minor << L'.' << build << L'.' << revision;
    return stream.str();
}
std::wstring ExtractLeadingVersion(const std::wstring& text) {
    std::wstring out;
    int dots = 0;
    for (const wchar_t ch : text) {
        if (ch >= L'0' && ch <= L'9') {
            out.push_back(ch);
            continue;
        }
        if (ch == L'.' && !out.empty() && out.back() != L'.') {
            out.push_back(ch);
            ++dots;
            continue;
        }
        break;
    }
    while (!out.empty() && out.back() == L'.') {
        out.pop_back();
    }
    if (dots != 3) {
        return {};
    }
    return out;
}
bool ParseVersion(
    const std::wstring& text,
    DWORD& major,
    DWORD& minor,
    DWORD& build,
    DWORD& revision) {
    unsigned int a = 0;
    unsigned int b = 0;
    unsigned int c = 0;
    unsigned int d = 0;
    if (swscanf_s(text.c_str(), L"%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return false;
    }
    major = a;
    minor = b;
    build = c;
    revision = d;
    return true;
}
void AddUnique(std::vector<std::wstring>& values, const std::wstring& value) {
    if (value.empty()) {
        return;
    }
    for (const std::wstring& existing : values) {
        if (existing == value) {
            return;
        }
    }
    values.push_back(value);
}
std::wstring OsBuildAlias(DWORD revision) {
    using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        return {};
    }
    auto rtl_get_version = reinterpret_cast<RtlGetVersionFn>(
        GetProcAddress(ntdll, "RtlGetVersion"));
    if (rtl_get_version == nullptr) {
        return {};
    }

    OSVERSIONINFOW info{};
    info.dwOSVersionInfoSize = sizeof(info);
    if (rtl_get_version(&info) != 0) {
        return {};
    }
    return FormatVersion(
        info.dwMajorVersion,
        info.dwMinorVersion,
        info.dwBuildNumber,
        revision);
}
}
IniSupport::IniSupport(AppPaths paths) : paths_(std::move(paths)) {}

std::wstring IniSupport::ReadTermsrvFileVersion() const {
    const std::wstring path = TermsrvPath();
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
    if (size == 0) {
        return {};
    }

    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, buffer.data())) {
        return {};
    }

    struct LANGANDCODEPAGE {
        WORD language;
        WORD codePage;
    };

    LANGANDCODEPAGE* translate = nullptr;
    UINT translateBytes = 0;
    if (!VerQueryValueW(
            buffer.data(),
            L"\\VarFileInfo\\Translation",
            reinterpret_cast<LPVOID*>(&translate),
            &translateBytes) ||
        translateBytes < sizeof(LANGANDCODEPAGE)) {
        return {};
    }

    wchar_t subBlock[64]{};
    swprintf_s(
        subBlock,
        L"\\StringFileInfo\\%04x%04x\\FileVersion",
        translate[0].language,
        translate[0].codePage);

    LPVOID value = nullptr;
    UINT length = 0;
    if (!VerQueryValueW(buffer.data(), subBlock, &value, &length) ||
        value == nullptr) {
        return {};
    }
    return ExtractLeadingVersion(static_cast<const wchar_t*>(value));
}
std::wstring IniSupport::ReadTermsrvBinaryVersion() const {
    const std::wstring path = TermsrvPath();
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
    if (size == 0) {
        return {};
    }

    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, buffer.data())) {
        return {};
    }

    VS_FIXEDFILEINFO* info = nullptr;
    UINT length = 0;
    if (!VerQueryValueW(
            buffer.data(),
            L"\\",
            reinterpret_cast<LPVOID*>(&info),
            &length) ||
        info == nullptr) {
        return {};
    }

    return FormatVersion(
        HIWORD(info->dwFileVersionMS),
        LOWORD(info->dwFileVersionMS),
        HIWORD(info->dwFileVersionLS),
        LOWORD(info->dwFileVersionLS));
}
StatusItem IniSupport::CheckIniSupportsCurrentTermsrv() const {
    StatusItem item;
    item.id = L"ini_support";
    item.title = L"INI supports termsrv";

    const std::wstring binary = ReadTermsrvBinaryVersion();
    const std::wstring file_version = ReadTermsrvFileVersion();
    if (binary.empty() && file_version.empty()) {
        item.level = StatusLevel::Error;
        item.detail = L"Cannot read termsrv.dll version";
        return item;
    }

    std::wstring iniPath = paths_.installedIni;
    if (!FileExists(iniPath)) {
        iniPath = paths_.sourceIni;
    }
    if (!FileExists(iniPath)) {
        item.level = StatusLevel::Error;
        item.detail = L"rdpwrap.ini not found";
        return item;
    }

    std::vector<std::wstring> candidates;
    AddUnique(candidates, binary);
    AddUnique(candidates, file_version);

    DWORD bin_major = 0;
    DWORD bin_minor = 0;
    DWORD bin_build = 0;
    DWORD bin_revision = 0;
    DWORD str_major = 0;
    DWORD str_minor = 0;
    DWORD str_build = 0;
    DWORD str_revision = 0;
    const bool have_binary =
        ParseVersion(binary, bin_major, bin_minor, bin_build, bin_revision);
    const bool have_string = ParseVersion(
        file_version,
        str_major,
        str_minor,
        str_build,
        str_revision);

    if (have_binary && have_string) {
        AddUnique(
            candidates,
            FormatVersion(str_major, str_minor, bin_build, bin_revision));
    }
    if (have_binary) {
        AddUnique(candidates, OsBuildAlias(bin_revision));
    } else if (have_string) {
        AddUnique(candidates, OsBuildAlias(str_revision));
    }

    for (const std::wstring& section : candidates) {
        if (FileContainsSection(iniPath, section)) {
            item.level = StatusLevel::Ok;
            item.detail = L"Found [" + section + L"]";
            return item;
        }
    }

    item.level = StatusLevel::Error;
    if (!candidates.empty()) {
        item.detail = L"Missing [" + candidates.front() + L"] in ini";
    } else {
        item.detail = L"Missing section in ini";
    }
    return item;
}
}