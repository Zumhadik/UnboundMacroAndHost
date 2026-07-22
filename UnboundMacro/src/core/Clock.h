#pragma once

#include <cstdint>
#include <windows.h>

namespace Clock {
inline std::uint64_t Frequency()
{
    static LARGE_INTEGER freq = {};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    return static_cast<std::uint64_t>(freq.QuadPart);
}

inline std::uint64_t NowMs()
{
    LARGE_INTEGER counter = {};
    QueryPerformanceCounter(&counter);
    return (static_cast<std::uint64_t>(counter.QuadPart) * 1000ULL) /
        Frequency();
}

inline std::uint64_t NowUs()
{
    LARGE_INTEGER counter = {};
    QueryPerformanceCounter(&counter);
    return (static_cast<std::uint64_t>(counter.QuadPart) * 1000000ULL) /
        Frequency();
}
}
