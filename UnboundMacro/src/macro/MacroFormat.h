#pragma once

#include "MacroEvent.h"

#include <cstdint>
#include <string>

class MacroFormat {
public:
    static bool Save(
        const std::wstring& path,
        const MacroEvents& events,
        std::uint32_t duration_ms);
    static bool Load(
        const std::wstring& path,
        MacroEvents* events,
        std::uint32_t* duration_ms = nullptr);
    static bool WriteEmpty(const std::wstring& path);
    static bool ReadDuration(const std::wstring& path, std::uint32_t* duration_ms);
};
