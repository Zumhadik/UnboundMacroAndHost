#include "rdphost/WrapperInstall.h"

namespace rdphost {

WrapperInstall::WrapperInstall(AppPaths paths)
    : files_(std::move(paths)),
      registry_(),
      firewall_() {}

OperationResult WrapperInstall::Install() {
    OperationResult stop = term_service_.Stop();
    if (!stop.ok) {
        const ServiceState state = term_service_.QueryState();
        if (state != ServiceState::Stopped &&
            state != ServiceState::StopPending) {
            return stop;
        }
    }

    OperationResult files = files_.EnsureInstalledCopies();
    if (!files.ok) {
        return files;
    }

    OperationResult reg = registry_.ApplyInstallKeys(
        L"%ProgramFiles%\\RDP Wrapper\\rdpwrap.dll");
    if (!reg.ok) {
        return reg;
    }

    OperationResult firewall = firewall_.EnsureRdpAllowRules();
    if (!firewall.ok) {
        return firewall;
    }

    OperationResult autoStart = term_service_.SetStartTypeAutomatic();
    if (!autoStart.ok) {
        return autoStart;
    }

    OperationResult start = term_service_.Start();
    if (!start.ok) {
        return start;
    }

    return {true, L"RDP Wrapper installed and TermService running"};
}

}
