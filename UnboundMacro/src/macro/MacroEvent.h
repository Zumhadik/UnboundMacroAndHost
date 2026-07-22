#pragma once

#include <cstdint>
#include <vector>

enum class MacroEventType : std::uint8_t {
    Move = 1,
    BtnDown = 2,
    BtnUp = 3,
    Wheel = 4,
    KeyDown = 5,
    KeyUp = 6
};

enum class MouseBtn : std::uint8_t {
    Left = 1,
    Right = 2,
    Middle = 3
};

struct MacroEvent {
    std::uint32_t dt_ms = 0;
    MacroEventType type = MacroEventType::Move;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t wheel = 0;
    std::uint16_t vk = 0;
    std::uint8_t button = 0;
};

using MacroEvents = std::vector<MacroEvent>;
