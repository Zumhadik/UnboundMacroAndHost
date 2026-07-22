#pragma once

#include "rdphost/HealthCheck.h"
#include "rdphost/Paths.h"
#include "rdphost/PortListener.h"
#include "rdphost/SessionLauncher.h"
#include "rdphost/Types.h"
#include "rdphost/WrapperInstall.h"

namespace rdphost {

class AppController {
public:
    explicit AppController(AppPaths paths);

    StatusList RunDiagnostics();
    OperationResult EnsureInstalled();
    OperationResult PrepareAndLaunch(const SessionOptions& options,
                                     std::uint32_t listenTimeoutMs);

private:
    OperationResult WaitForListener(std::uint32_t timeoutMs);
    OperationResult LaunchSession(const SessionOptions& options);

    AppPaths paths_;
    HealthCheck health_;
    WrapperInstall installer_;
    PortListener port_listener_;
    SessionLauncher session_launcher_;
};

}