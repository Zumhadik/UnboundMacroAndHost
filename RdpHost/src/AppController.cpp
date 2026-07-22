#include "rdphost/AppController.h"

#include "rdphost/WinUtils.h"

namespace rdphost {

AppController::AppController(AppPaths paths)
    : paths_(std::move(paths)),
      health_(paths_),
      installer_(paths_),
      port_listener_(),
      session_launcher_(paths_) {}

StatusList AppController::RunDiagnostics() {
    return health_.RunAll();
}

OperationResult AppController::EnsureInstalled() {
    if (!WinUtils::IsProcessElevated()) {
        return {false, L"Run RdpHost as Administrator to install"};
    }
    return installer_.Install();
}

OperationResult AppController::WaitForListener(std::uint32_t timeoutMs) {
    return port_listener_.WaitUntilListening(timeoutMs);
}

OperationResult AppController::LaunchSession(const SessionOptions& options) {
    return session_launcher_.WriteAndLaunch(options);
}

OperationResult AppController::PrepareAndLaunch(
    const SessionOptions& options,
    std::uint32_t listenTimeoutMs) {
    StatusList status = health_.RunAll();
    if (!health_.AllCriticalOk(status)) {
        OperationResult install = EnsureInstalled();
        if (!install.ok) {
            return install;
        }
    }

    OperationResult listen = WaitForListener(listenTimeoutMs);
    if (!listen.ok) {
        OperationResult start = TermService().Start();
        if (!start.ok) {
            return listen;
        }
        listen = WaitForListener(listenTimeoutMs);
        if (!listen.ok) {
            return listen;
        }
    }

    return LaunchSession(options);
}

}
