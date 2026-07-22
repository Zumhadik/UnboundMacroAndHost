#pragma once

#include "rdphost/Paths.h"
#include "rdphost/Types.h"

namespace rdphost {

class WrapperFiles {
public:
    explicit WrapperFiles(AppPaths paths);

    StatusItem CheckInstalledFiles() const;
    OperationResult EnsureInstalledCopies();

private:
    AppPaths paths_;
};
}