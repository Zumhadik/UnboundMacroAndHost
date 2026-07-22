#pragma once

#include <string>
#include <vector>

namespace rdphost {

enum class StatusLevel {
    Ok,
    Warn,
    Error,
    Unknown,
};

struct StatusItem {
    std::wstring id;
    std::wstring title;
    std::wstring detail;
    StatusLevel level = StatusLevel::Unknown;
};

struct OperationResult {
    bool ok = false;
    std::wstring message;
};

using StatusList = std::vector<StatusItem>;

}