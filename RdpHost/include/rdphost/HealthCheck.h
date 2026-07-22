#pragma once

#include "rdphost/IniSupport.h"
#include "rdphost/Paths.h"
#include "rdphost/PortListener.h"
#include "rdphost/RegistryConfig.h"
#include "rdphost/TermService.h"
#include "rdphost/Types.h"
#include "rdphost/WrapperFiles.h"

namespace rdphost {

class HealthCheck {
public:
    explicit HealthCheck(AppPaths paths);

    StatusList RunAll() const;
    bool AllCriticalOk(const StatusList& items) const;

private:
    TermService term_service_;
    WrapperFiles files_;
    RegistryConfig registry_;
    PortListener port_listener_;
    IniSupport ini_support_;
};

}