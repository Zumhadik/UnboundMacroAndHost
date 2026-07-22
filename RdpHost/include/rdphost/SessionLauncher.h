#pragma once

#include "rdphost/Paths.h"
#include "rdphost/Types.h"

#include <cstdint>
#include <string>

namespace rdphost {

struct SessionOptions {
    std::wstring host = L"127.0.0.2";
    std::uint16_t port = 3389;
    std::wstring username;
    std::uint32_t desktopWidth = 1366;
    std::uint32_t desktopHeight = 768;
    bool fullscreen = false;
};

class SessionLauncher {
public:
    explicit SessionLauncher(AppPaths paths);

    OperationResult WriteRdpFile(const SessionOptions& options) const;
    OperationResult LaunchMstsc() const;
    OperationResult WriteAndLaunch(const SessionOptions& options) const;

private:
    AppPaths paths_;
};

}