#include "rdphost/TermService.h"

#include "rdphost/WinUtils.h"

#include <windows.h>

#include <chrono>
#include <thread>

namespace rdphost {
namespace {

ServiceState FromWinState(DWORD state) {
    switch (state) {
        case SERVICE_STOPPED:
            return ServiceState::Stopped;
        case SERVICE_START_PENDING:
            return ServiceState::StartPending;
        case SERVICE_STOP_PENDING:
            return ServiceState::StopPending;
        case SERVICE_RUNNING:
            return ServiceState::Running;
        case SERVICE_CONTINUE_PENDING:
            return ServiceState::ContinuePending;
        case SERVICE_PAUSE_PENDING:
            return ServiceState::PausePending;
        case SERVICE_PAUSED:
            return ServiceState::Paused;
        default:
            return ServiceState::Unknown;
    }
}
const wchar_t* ToText(ServiceState state) {
    switch (state) {
        case ServiceState::Stopped:
            return L"Stopped";
        case ServiceState::StartPending:
            return L"StartPending";
        case ServiceState::StopPending:
            return L"StopPending";
        case ServiceState::Running:
            return L"Running";
        case ServiceState::ContinuePending:
            return L"ContinuePending";
        case ServiceState::PausePending:
            return L"PausePending";
        case ServiceState::Paused:
            return L"Paused";
        case ServiceState::Unknown:
            return L"Unknown";
    }
    return L"Unknown";
}
class ServiceHandle {
public:
    explicit ServiceHandle(SC_HANDLE handle) : handle_(handle) {}
    ~ServiceHandle() {
        if (handle_ != nullptr) {
            CloseServiceHandle(handle_);
        }
    }

    ServiceHandle(const ServiceHandle&) = delete;
    ServiceHandle& operator=(const ServiceHandle&) = delete;

    SC_HANDLE Get() const {
        return handle_;
    }

    bool Valid() const {
        return handle_ != nullptr;
    }

private:
    SC_HANDLE handle_ = nullptr;
};

bool QueryStatus(SC_HANDLE service, SERVICE_STATUS_PROCESS* status) {
    DWORD bytes = 0;
    return QueryServiceStatusEx(
               service,
               SC_STATUS_PROCESS_INFO,
               reinterpret_cast<LPBYTE>(status),
               sizeof(*status),
               &bytes) == TRUE;
}
}
OperationResult TermService::Stop() {
    ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!manager.Valid()) {
        return WinUtils::LastErrorAsResult(L"OpenSCManager");
    }

    ServiceHandle service(
        OpenServiceW(manager.Get(), kServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS));
    if (!service.Valid()) {
        return WinUtils::LastErrorAsResult(L"OpenService(TermService)");
    }

    SERVICE_STATUS_PROCESS status{};
    if (!QueryStatus(service.Get(), &status)) {
        return WinUtils::LastErrorAsResult(L"QueryServiceStatusEx");
    }

    if (status.dwCurrentState == SERVICE_STOPPED) {
        return {true, L"TermService already stopped"};
    }

    SERVICE_STATUS control{};
    if (!ControlService(service.Get(), SERVICE_CONTROL_STOP, &control)) {
        const DWORD error = GetLastError();
        if (error != ERROR_SERVICE_NOT_ACTIVE) {
            SetLastError(error);
            return WinUtils::LastErrorAsResult(L"ControlService(STOP)");
        }
    }

    return WaitUntil(ServiceState::Stopped, 15000);
}
OperationResult TermService::Start() {
    ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!manager.Valid()) {
        return WinUtils::LastErrorAsResult(L"OpenSCManager");
    }

    ServiceHandle service(
        OpenServiceW(manager.Get(), kServiceName, SERVICE_START | SERVICE_QUERY_STATUS));
    if (!service.Valid()) {
        return WinUtils::LastErrorAsResult(L"OpenService(TermService)");
    }

    SERVICE_STATUS_PROCESS status{};
    if (!QueryStatus(service.Get(), &status)) {
        return WinUtils::LastErrorAsResult(L"QueryServiceStatusEx");
    }

    if (status.dwCurrentState == SERVICE_RUNNING) {
        return {true, L"TermService already running"};
    }

    if (!StartServiceW(service.Get(), 0, nullptr)) {
        const DWORD error = GetLastError();
        if (error != ERROR_SERVICE_ALREADY_RUNNING) {
            SetLastError(error);
            return WinUtils::LastErrorAsResult(L"StartService");
        }
    }

    return WaitUntil(ServiceState::Running, 20000);
}
OperationResult TermService::SetStartTypeAutomatic() {
    ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!manager.Valid()) {
        return WinUtils::LastErrorAsResult(L"OpenSCManager");
    }

    ServiceHandle service(
        OpenServiceW(manager.Get(), kServiceName, SERVICE_CHANGE_CONFIG));
    if (!service.Valid()) {
        return WinUtils::LastErrorAsResult(L"OpenService(TermService)");
    }

    if (!ChangeServiceConfigW(
            service.Get(),
            SERVICE_NO_CHANGE,
            SERVICE_AUTO_START,
            SERVICE_NO_CHANGE,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr)) {
        return WinUtils::LastErrorAsResult(L"ChangeServiceConfig");
    }

    return {true, L"TermService start type set to Automatic"};
}
ServiceState TermService::QueryState() const {
    ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!manager.Valid()) {
        return ServiceState::Unknown;
    }

    ServiceHandle service(
        OpenServiceW(manager.Get(), kServiceName, SERVICE_QUERY_STATUS));
    if (!service.Valid()) {
        return ServiceState::Unknown;
    }

    SERVICE_STATUS_PROCESS status{};
    if (!QueryStatus(service.Get(), &status)) {
        return ServiceState::Unknown;
    }
    return FromWinState(status.dwCurrentState);
}
OperationResult TermService::WaitUntil(ServiceState desired,
                                       std::uint32_t timeoutMs) const {
    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        const ServiceState state = QueryState();
        if (state == desired) {
            OperationResult result;
            result.ok = true;
            result.message = std::wstring(L"TermService is ") + ToText(desired);
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    OperationResult result;
    result.ok = false;
    result.message = std::wstring(L"Timeout waiting for TermService ") +
                     ToText(desired) + L", current=" + ToText(QueryState());
    return result;
}
StatusItem TermService::CheckStatus() const {
    StatusItem item;
    item.id = L"termservice";
    item.title = L"TermService";

    const ServiceState state = QueryState();
    item.detail = ToText(state);
    item.level =
        (state == ServiceState::Running) ? StatusLevel::Ok : StatusLevel::Error;
    return item;
}
}