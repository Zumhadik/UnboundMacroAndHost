#pragma once

#include "rdphost/FirewallConfig.h"
#include "rdphost/Paths.h"
#include "rdphost/RegistryConfig.h"
#include "rdphost/TermService.h"
#include "rdphost/Types.h"
#include "rdphost/WrapperFiles.h"

namespace rdphost {

class WrapperInstall {
public:
    explicit WrapperInstall(AppPaths paths);

    OperationResult Install();

private:
    TermService term_service_;
    WrapperFiles files_;
    RegistryConfig registry_;
    FirewallConfig firewall_;
};
}