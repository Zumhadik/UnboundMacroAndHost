#include "InputHook.h"

#include "MacroRecorder.h"
#include "../core/Constants.h"
#include "../keybinds/KeybindManager.h"

InputHook* InputHook::instance_ = nullptr;

bool InputHook::Install(HWND notify_hwnd)
{
    Uninstall();
    instance_ = this;
    notify_hwnd_ = notify_hwnd;
    record_down_ = false;
    play_down_ = false;

    keyboard_hook_ = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        KeyboardProc,
        GetModuleHandleW(nullptr),
        0);
    mouse_hook_ = SetWindowsHookExW(
        WH_MOUSE_LL,
        MouseProc,
        GetModuleHandleW(nullptr),
        0);

    if (!keyboard_hook_ || !mouse_hook_) {
        Uninstall();
        return false;
    }
    return true;
}

void InputHook::Uninstall()
{
    if (keyboard_hook_) {
        UnhookWindowsHookEx(keyboard_hook_);
        keyboard_hook_ = nullptr;
    }
    if (mouse_hook_) {
        UnhookWindowsHookEx(mouse_hook_);
        mouse_hook_ = nullptr;
    }
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

void InputHook::SetRecorder(MacroRecorder* recorder)
{
    recorder_ = recorder;
}

void InputHook::SetHotkeys(int record_vk, int play_vk)
{
    record_vk_ = record_vk;
    play_vk_ = play_vk;
    record_down_ = false;
    play_down_ = false;
}

void InputHook::SetCaptureEnabled(bool enabled)
{
    capture_enabled_ = enabled;
}

void InputHook::SetHotkeysEnabled(bool enabled)
{
    hotkeys_enabled_ = enabled;
    if (!enabled) {
        record_down_ = false;
        play_down_ = false;
    }
}

LRESULT CALLBACK InputHook::KeyboardProc(
    int code,
    WPARAM wparam,
    LPARAM lparam)
{
    if (code == HC_ACTION && instance_) {
        const auto* info = reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam);
        const bool down =
            wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN;
        const bool up =
            wparam == WM_KEYUP || wparam == WM_SYSKEYUP;
        const bool extended = (info->flags & LLKHF_EXTENDED) != 0;
        const int vk = KeybindManager::NormalizeVirtualKey(
            static_cast<int>(info->vkCode),
            extended);

        if (vk == instance_->record_vk_) {
            if (down) {
                if (!instance_->record_down_) {
                    instance_->record_down_ = true;
                    if (instance_->hotkeys_enabled_ &&
                        instance_->notify_hwnd_) {
                        PostMessageW(
                            instance_->notify_hwnd_,
                            WM_TOGGLE_RECORD,
                            0,
                            0);
                    }
                }
            } else if (up) {
                instance_->record_down_ = false;
            }
        } else if (vk == instance_->play_vk_) {
            if (down) {
                if (!instance_->play_down_) {
                    instance_->play_down_ = true;
                    if (instance_->hotkeys_enabled_ &&
                        instance_->notify_hwnd_) {
                        PostMessageW(
                            instance_->notify_hwnd_,
                            WM_TOGGLE_PLAY,
                            0,
                            0);
                    }
                }
            } else if (up) {
                instance_->play_down_ = false;
            }
        } else if (
            instance_->capture_enabled_ &&
            instance_->recorder_ &&
            (down || up)) {
            instance_->recorder_->OnKey(vk, down);
        }
    }

    return CallNextHookEx(
        instance_ ? instance_->keyboard_hook_ : nullptr,
        code,
        wparam,
        lparam);
}

LRESULT CALLBACK InputHook::MouseProc(
    int code,
    WPARAM wparam,
    LPARAM lparam)
{
    if (code == HC_ACTION &&
        instance_ &&
        instance_->capture_enabled_ &&
        instance_->recorder_) {
        const auto* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lparam);
        const int x = info->pt.x;
        const int y = info->pt.y;

        switch (wparam) {
        case WM_MOUSEMOVE:
            instance_->recorder_->OnMouseMove(x, y);
            break;
        case WM_LBUTTONDOWN:
            instance_->recorder_->OnMouseButton(1, true, x, y);
            break;
        case WM_LBUTTONUP:
            instance_->recorder_->OnMouseButton(1, false, x, y);
            break;
        case WM_RBUTTONDOWN:
            instance_->recorder_->OnMouseButton(2, true, x, y);
            break;
        case WM_RBUTTONUP:
            instance_->recorder_->OnMouseButton(2, false, x, y);
            break;
        case WM_MBUTTONDOWN:
            instance_->recorder_->OnMouseButton(3, true, x, y);
            break;
        case WM_MBUTTONUP:
            instance_->recorder_->OnMouseButton(3, false, x, y);
            break;
        case WM_MOUSEWHEEL: {
            const short delta = HIWORD(info->mouseData);
            const int notches = delta / WHEEL_DELTA;
            if (notches != 0) {
                instance_->recorder_->OnWheel(notches, x, y);
            }
            break;
        }
        default:
            break;
        }
    }

    return CallNextHookEx(
        instance_ ? instance_->mouse_hook_ : nullptr,
        code,
        wparam,
        lparam);
}
