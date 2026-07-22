#pragma once

#include "../core/Types.h"

#include <windows.h>

#include <cstdint>
#include <string>

constexpr int VK_NUMENTER = 0x10E;

class KeybindManager {
public:
    static constexpr UINT WM_KEYBIND = WM_APP + 40;

    void BeginCapture(KeybindSlot slot, HWND notify_hwnd);
    void CancelCapture();
    bool IsCapturing() const;
    bool IsArmed() const;
    KeybindSlot CapturingSlot() const;

    bool PollAndApply(KeybindConfig* config);
    bool ApplyVirtualKey(int vk, KeybindConfig* config);

    static int NormalizeVirtualKey(int vk, bool extended);
    static bool IsAllowedVirtualKey(int vk);
    static std::wstring VirtualKeyToLabel(int vk);

private:
    void InstallHook();
    void RemoveHook();
    void DrainAsyncKeyState() const;

    static LRESULT CALLBACK LowLevelKeyboardProc(
        int code,
        WPARAM wparam,
        LPARAM lparam);

    bool capturing_ = false;
    KeybindSlot slot_ = KeybindSlot::RecordStop;
    std::uint64_t arm_at_ms_ = 0;
    HWND notify_hwnd_ = nullptr;
    HHOOK hook_ = nullptr;
    int pending_vk_ = 0;
    bool pending_cancel_ = false;

    static KeybindManager* active_;
};
