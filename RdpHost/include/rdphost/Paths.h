#pragma once

#include <string>

namespace rdphost {

struct AppPaths {
    std::wstring appDirectory;
    std::wstring sourceDll;
    std::wstring sourceIni;
    std::wstring installDirectory;
    std::wstring installedDll;
    std::wstring installedIni;
    std::wstring generatedRdpFile;
};

class Paths {
public:
    static AppPaths Resolve(const std::wstring& appDirectory);
    static std::wstring ProgramFilesRdpWrapperDirectory();
};

}