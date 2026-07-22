#pragma once

#include "../macro/MacroEvent.h"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

class MacroPlayer {
public:
    ~MacroPlayer();

    bool Load(const std::wstring& path);
    bool Start(HWND notify_hwnd, bool loop);
    void Stop();
    bool IsPlaying() const;
    std::uint64_t ElapsedMs() const;

private:
    void ThreadMain();
    bool WaitUntilMs(std::uint64_t start_ms, std::uint64_t target_ms);
    bool WaitMoveInterpolated(
        std::uint64_t start_ms,
        std::uint64_t from_abs_ms,
        std::uint64_t target_ms,
        int from_x,
        int from_y,
        int to_x,
        int to_y);
    void PlayEvent(const MacroEvent& event);
    void MoveTo(int x, int y);
    void ResetCursorState();
    void BeginInputProfile();
    void EndInputProfile();
    static void SendMouseAbsolute(int x, int y);
    static void SendMouseRelative(int dx, int dy);
    static void SendMouseButton(int button, bool down);
    static void SendWheel(int notches);
    static void SendKey(int vk, bool down);
    static bool KeyNeedsExtended(int vk);
    static WORD VkToScan(int vk, bool* extended);

    MacroEvents events_;
    std::vector<std::uint64_t> absolute_ms_;
    HWND notify_hwnd_ = nullptr;
    bool loop_ = false;
    std::atomic<bool> playing_{false};
    std::atomic<bool> stop_requested_{false};
    std::thread worker_;
    std::uint64_t start_ms_ = 0;
    int cursor_x_ = 0;
    int cursor_y_ = 0;
    bool cursor_ready_ = false;
    bool saved_mouse_ = false;
    int saved_mouse_params_[3] = {};
    int saved_mouse_speed_ = 10;
};
