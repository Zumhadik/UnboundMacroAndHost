#pragma once

#include "rdphost/Types.h"

#include <cstdint>
#include <string>

namespace rdphost {

enum class ServiceState {
    Stopped,
    StartPending,
    StopPending,
    Running,
    ContinuePending,
    PausePending,
    Paused,
    Unknown,
};

class TermService {
public:
    static constexpr const wchar_t* kServiceName = L"TermService";

    OperationResult Stop();
    OperationResult Start();
    OperationResult SetStartTypeAutomatic();

    ServiceState QueryState() const;
    OperationResult WaitUntil(ServiceState desired, std::uint32_t timeoutMs) const;
    StatusItem CheckStatus() const;
};

}