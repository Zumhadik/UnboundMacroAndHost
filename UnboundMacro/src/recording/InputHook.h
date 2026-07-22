#pragma once

#include <windows.h>

class MacroRecorder;

class InputHook {
public:
    bool Install(HWND notify_hwnd);
    void Uninstall();

    void SetRecorder(MacroRecorder* recorder);
    void SetHotkeys(int record_vk, int play_vk);
    void SetCaptureEnabled(bool enabled);
    void SetHotkeysEnabled(bool enabled);

private:
    static LRESULT CALLBACK KeyboardProc(int code, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK MouseProc(int code, WPARAM wparam, LPARAM lparam);

    HWND notify_hwnd_ = nullptr;
    MacroRecorder* recorder_ = nullptr;
    HHOOK keyboard_hook_ = nullptr;
    HHOOK mouse_hook_ = nullptr;
    int record_vk_ = 0;
    int play_vk_ = 0;
    bool capture_enabled_ = false;
    bool hotkeys_enabled_ = true;
    bool record_down_ = false;
    bool play_down_ = false;

    static InputHook* instance_;
};
