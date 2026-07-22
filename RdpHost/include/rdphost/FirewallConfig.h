#pragma once

#include "rdphost/Types.h"

namespace rdphost {

class FirewallConfig {
public:
    OperationResult EnsureRdpAllowRules();
};

}