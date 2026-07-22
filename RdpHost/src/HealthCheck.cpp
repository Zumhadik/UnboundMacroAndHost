#include "rdphost/HealthCheck.h"

#include "rdphost/WinUtils.h"

namespace rdphost {
namespace {

StatusItem MakeWrapperStatus(const WrapperFiles& files,
                             const RegistryConfig& registry) {
    const StatusItem installed = files.CheckInstalledFiles();
    const StatusItem serviceDll = registry.CheckServiceDll();

    StatusItem item;
    item.id = L"wrapper";
    item.title = L"Wrapper";

    if (installed.level == StatusLevel::Ok &&
        serviceDll.level == StatusLevel::Ok) {
        item.level = StatusLevel::Ok;
        item.detail = L"Installed and ServiceDll points to rdpwrap.dll";
        return item;
    }

    item.level = StatusLevel::Error;
    if (installed.level != StatusLevel::Ok &&
        serviceDll.level != StatusLevel::Ok) {
        item.detail = L"Not installed";
    } else if (installed.level != StatusLevel::Ok) {
        item.detail = L"Files missing in Program Files";
    } else {
        item.detail = L"ServiceDll is not rdpwrap.dll";
    }
    return item;
}

StatusItem MakeServiceStatus(const TermService& term_service) {
    StatusItem item = term_service.CheckStatus();
    item.id = L"service";
    item.title = L"TermService";
    if (item.level == StatusLevel::Ok) {
        item.detail = L"Running";
    } else {
        item.detail = L"Not running (" + item.detail + L")";
    }
    return item;
}

StatusItem MakeListenerStatus(const PortListener& port_listener) {
    StatusItem item = port_listener.CheckStatus();
    item.id = L"listener";
    item.title = L"Listener";
    if (item.level == StatusLevel::Ok) {
        item.detail = L"Port 3389 is listening";
    } else {
        item.detail = L"Port 3389 is not listening";
    }
    return item;
}

StatusItem MakeIniStatus(const IniSupport& ini_support) {
    StatusItem item = ini_support.CheckIniSupportsCurrentTermsrv();
    item.id = L"ini";
    item.title = L"INI";
    if (item.level == StatusLevel::Ok) {
        if (item.detail.rfind(L"Found ", 0) == 0) {
            item.detail = L"Supports " + item.detail.substr(6);
        } else {
            item.detail = L"Supports current termsrv.dll";
        }
    }
    return item;
}

StatusItem MakeAdminStatus() {
    StatusItem item;
    item.id = L"admin";
    item.title = L"Admin";
    if (WinUtils::IsProcessElevated()) {
        item.level = StatusLevel::Ok;
        item.detail = L"Running as Administrator";
    } else {
        item.level = StatusLevel::Warn;
        item.detail = L"Install needs Administrator";
    }
    return item;
}

}

HealthCheck::HealthCheck(AppPaths paths)
    : files_(paths),
      registry_(),
      port_listener_(),
      ini_support_(std::move(paths)) {}

StatusList HealthCheck::RunAll() const {
    return {
        MakeWrapperStatus(files_, registry_),
        MakeServiceStatus(term_service_),
        MakeListenerStatus(port_listener_),
        MakeIniStatus(ini_support_),
        MakeAdminStatus(),
    };
}

bool HealthCheck::AllCriticalOk(const StatusList& items) const {
    for (const StatusItem& item : items) {
        if (item.id == L"admin") {
            continue;
        }
        if (item.level == StatusLevel::Error) {
            return false;
        }
    }
    return true;
}

}
