#include "rdphost/Paths.h"

#include "rdphost/WinUtils.h"

#include <windows.h>

namespace rdphost {
namespace {

std::wstring JoinPath(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }

    std::wstring result = left;
    if (result.back() != L'\\' && result.back() != L'/') {
        result.push_back(L'\\');
    }
    result += right;
    return result;
}
std::wstring ParentDirectory(const std::wstring& path) {
    std::wstring result = path;
    while (!result.empty() &&
           (result.back() == L'\\' || result.back() == L'/')) {
        result.pop_back();
    }

    const size_t pos = result.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return {};
    }
    return result.substr(0, pos);
}
bool FileExists(const std::wstring& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES &&
           (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
bool HasBundle(const std::wstring& directory) {
    return FileExists(JoinPath(directory, L"rdpwrap.dll")) &&
           FileExists(JoinPath(directory, L"rdpwrap.ini"));
}
std::wstring FindBundleDirectory(const std::wstring& appDirectory) {
    const std::wstring bases[] = {
        appDirectory,
        ParentDirectory(appDirectory),
        ParentDirectory(ParentDirectory(appDirectory)),
        ParentDirectory(ParentDirectory(ParentDirectory(appDirectory))),
    };

    for (const std::wstring& base : bases) {
        if (base.empty()) {
            continue;
        }
        const std::wstring res = JoinPath(base, L"res");
        if (HasBundle(res)) {
            return res;
        }
        if (HasBundle(base)) {
            return base;
        }
    }

    return JoinPath(appDirectory, L"res");
}
}
std::wstring Paths::ProgramFilesRdpWrapperDirectory() {
    return JoinPath(WinUtils::ExpandEnv(L"%ProgramFiles%"), L"RDP Wrapper");
}
AppPaths Paths::Resolve(const std::wstring& appDirectory) {
    AppPaths paths;
    paths.appDirectory = appDirectory;

    const std::wstring bundle = FindBundleDirectory(appDirectory);
    paths.sourceDll = JoinPath(bundle, L"rdpwrap.dll");
    paths.sourceIni = JoinPath(bundle, L"rdpwrap.ini");

    paths.installDirectory = ProgramFilesRdpWrapperDirectory();
    paths.installedDll = JoinPath(paths.installDirectory, L"rdpwrap.dll");
    paths.installedIni = JoinPath(paths.installDirectory, L"rdpwrap.ini");
    paths.generatedRdpFile = JoinPath(appDirectory, L"rdphost-session.rdp");
    return paths;
}
}