#pragma once

#include "rdphost/Paths.h"
#include "rdphost/Types.h"

#include <string>

namespace rdphost {

class IniSupport {
public:
    explicit IniSupport(AppPaths paths);

    std::wstring ReadTermsrvFileVersion() const;
    std::wstring ReadTermsrvBinaryVersion() const;

    StatusItem CheckIniSupportsCurrentTermsrv() const;

private:
    AppPaths paths_;
};

}