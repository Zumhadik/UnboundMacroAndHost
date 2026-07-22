#pragma once

#include <cstdint>
#include <string>

enum class KeybindSlot {
    RecordStop,
    PlayStop
};

struct KeybindConfig {
    int record_vk = 0x77;
    int play_vk = 0x79;
};

struct AppState {
    std::wstring macro_path;
    bool has_file = false;
    bool loop_enabled = false;
    bool is_recording = false;
    bool is_playing = false;
    KeybindConfig keybinds;
    bool waiting_keybind = false;
    std::uint64_t elapsed_ms = 0;
};
