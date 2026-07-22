#include "rdphost/WrapperFiles.h"

#include "rdphost/WinUtils.h"

#include <windows.h>

namespace rdphost {
namespace {

bool FileExists(const std::wstring& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES &&
           (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

OperationResult EnsureDirectory(const std::wstring& directory) {
    if (directory.empty()) {
        return {false, L"Install directory is empty"};
    }

    const DWORD attrs = GetFileAttributesW(directory.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES &&
        (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return {true, L"Directory exists"};
    }

    if (CreateDirectoryW(directory.c_str(), nullptr) ||
        GetLastError() == ERROR_ALREADY_EXISTS) {
        return {true, L"Directory created"};
    }
    return WinUtils::LastErrorAsResult(L"CreateDirectory");
}

OperationResult CopyOne(const std::wstring& source,
                        const std::wstring& destination) {
    if (!CopyFileW(source.c_str(), destination.c_str(), FALSE)) {
        return WinUtils::LastErrorAsResult(L"CopyFile " + source);
    }
    return {true, L"Copied " + destination};
}

}

WrapperFiles::WrapperFiles(AppPaths paths) : paths_(std::move(paths)) {}

StatusItem WrapperFiles::CheckInstalledFiles() const {
    StatusItem item;
    item.id = L"installed_files";
    item.title = L"Installed dll/ini";

    const bool dll = FileExists(paths_.installedDll);
    const bool ini = FileExists(paths_.installedIni);
    if (dll && ini) {
        item.level = StatusLevel::Ok;
        item.detail = paths_.installDirectory;
    } else {
        item.level = StatusLevel::Error;
        item.detail = L"Not installed in " + paths_.installDirectory;
    }
    return item;
}

OperationResult WrapperFiles::EnsureInstalledCopies() {
    if (!FileExists(paths_.sourceDll) || !FileExists(paths_.sourceIni)) {
        return {false, L"Source rdpwrap.dll/ini not found"};
    }

    OperationResult dir = EnsureDirectory(paths_.installDirectory);
    if (!dir.ok) {
        return dir;
    }

    OperationResult dll = CopyOne(paths_.sourceDll, paths_.installedDll);
    if (!dll.ok) {
        return dll;
    }

    OperationResult ini = CopyOne(paths_.sourceIni, paths_.installedIni);
    if (!ini.ok) {
        return ini;
    }

    return {true, L"rdpwrap.dll and rdpwrap.ini installed"};
}

}
