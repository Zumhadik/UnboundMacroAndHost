#pragma once

#include "rdphost/Types.h"

#include <string>

namespace rdphost {

class RegistryConfig {
public:
    StatusItem CheckServiceDll() const;
    OperationResult ApplyInstallKeys(const std::wstring& serviceDllExpandPath);
    std::wstring ReadServiceDll() const;
};

}