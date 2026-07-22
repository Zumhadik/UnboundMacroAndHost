#pragma once

#include "../macro/MacroEvent.h"

#include <cstdint>
#include <string>

class MacroRecorder {
public:
    bool Start(const std::wstring& path);
    bool Stop();
    bool IsRecording() const;
    std::size_t EventCount() const;

    void OnMouseMove(int x, int y);
    void OnMouseButton(int button, bool down, int x, int y);
    void OnWheel(int notches, int x, int y);
    void OnKey(int vk, bool down);

    std::uint64_t ElapsedMs() const;
    std::uint64_t LastDurationMs() const;

private:
    void Push(MacroEvent event);
    std::uint32_t TakeDeltaMs();

    bool recording_ = false;
    std::wstring path_;
    MacroEvents events_;
    std::uint64_t start_ms_ = 0;
    std::uint64_t last_event_ms_ = 0;
    std::uint64_t last_move_ms_ = 0;
    std::uint64_t last_duration_ms_ = 0;
    int last_x_ = 0;
    int last_y_ = 0;
    bool has_pos_ = false;
};
