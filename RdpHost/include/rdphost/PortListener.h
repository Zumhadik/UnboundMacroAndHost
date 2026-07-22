#pragma once

#include "rdphost/Types.h"

#include <cstdint>

namespace rdphost {

class PortListener {
public:
    explicit PortListener(std::uint16_t port = 3389);

    bool IsListeningLocally() const;
    OperationResult WaitUntilListening(std::uint32_t timeoutMs) const;
    StatusItem CheckStatus() const;

private:
    std::uint16_t port_;
};

}