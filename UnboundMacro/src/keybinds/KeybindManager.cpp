#include "KeybindManager.h"

#include <windows.h>

namespace {
constexpr std::uint64_t CAPTURE_ARM_MS = 200;

bool IsOemKey(int vk)
{
    switch (vk) {
    case VK_OEM_1:
    case VK_OEM_2:
    case VK_OEM_3:
    case VK_OEM_4:
    case VK_OEM_5:
    case VK_OEM_6:
    case VK_OEM_7:
    case VK_OEM_8:
    case VK_OEM_PLUS:
    case VK_OEM_COMMA:
    case VK_OEM_MINUS:
    case VK_OEM_PERIOD:
    case VK_OEM_102:
    case VK_OEM_AX:
    case VK_OEM_CLEAR:
        return true;
    default:
        return false;
    }
}
}

KeybindManager* KeybindManager::active_ = nullptr;

void KeybindManager::DrainAsyncKeyState() const
{
    for (int vk = 1; vk <= 0xFE; ++vk) {
        GetAsyncKeyState(vk);
    }
}

void KeybindManager::InstallHook()
{
    if (hook_) {
        return;
    }
    active_ = this;
    hook_ = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        GetModuleHandleW(nullptr),
        0);
}

void KeybindManager::RemoveHook()
{
    if (hook_) {
        UnhookWindowsHookEx(hook_);
        hook_ = nullptr;
    }
    if (active_ == this) {
        active_ = nullptr;
    }
}

void KeybindManager::BeginCapture(KeybindSlot slot, HWND notify_hwnd)
{
    CancelCapture();
    capturing_ = true;
    slot_ = slot;
    notify_hwnd_ = notify_hwnd;
    pending_vk_ = 0;
    pending_cancel_ = false;
    DrainAsyncKeyState();
    arm_at_ms_ = GetTickCount64() + CAPTURE_ARM_MS;
    InstallHook();
}

void KeybindManager::CancelCapture()
{
    capturing_ = false;
    arm_at_ms_ = 0;
    pending_vk_ = 0;
    pending_cancel_ = false;
    RemoveHook();
}

bool KeybindManager::IsCapturing() const
{
    return capturing_;
}

bool KeybindManager::IsArmed() const
{
    return capturing_ && GetTickCount64() >= arm_at_ms_;
}

KeybindSlot KeybindManager::CapturingSlot() const
{
    return slot_;
}

int KeybindManager::NormalizeVirtualKey(int vk, bool extended)
{
    if (vk == VK_CLEAR) {
        return VK_NUMPAD5;
    }

    if (vk >= VK_NUMPAD0 && vk <= VK_DIVIDE) {
        return vk;
    }

    switch (vk) {
    case VK_INSERT:
        return extended ? VK_INSERT : VK_NUMPAD0;
    case VK_END:
        return extended ? VK_END : VK_NUMPAD1;
    case VK_DOWN:
        return extended ? VK_DOWN : VK_NUMPAD2;
    case VK_NEXT:
        return extended ? VK_NEXT : VK_NUMPAD3;
    case VK_LEFT:
        return extended ? VK_LEFT : VK_NUMPAD4;
    case VK_RIGHT:
        return extended ? VK_RIGHT : VK_NUMPAD6;
    case VK_HOME:
        return extended ? VK_HOME : VK_NUMPAD7;
    case VK_UP:
        return extended ? VK_UP : VK_NUMPAD8;
    case VK_PRIOR:
        return extended ? VK_PRIOR : VK_NUMPAD9;
    case VK_DELETE:
        return extended ? VK_DELETE : VK_DECIMAL;
    case VK_RETURN:
        return extended ? VK_NUMENTER : VK_RETURN;
    default:
        return vk;
    }
}

bool KeybindManager::IsAllowedVirtualKey(int vk)
{
    if (vk == VK_NUMENTER) {
        return true;
    }

    if (vk < 0x08 || vk > 0xFE) {
        return false;
    }

    if (IsOemKey(vk)) {
        return false;
    }

    if (vk >= '0' && vk <= '9') {
        return true;
    }
    if (vk >= 'A' && vk <= 'Z') {
        return true;
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_DIVIDE) {
        return true;
    }
    if (vk >= VK_F1 && vk <= VK_F24) {
        return true;
    }

    switch (vk) {
    case VK_BACK:
    case VK_TAB:
    case VK_RETURN:
    case VK_SPACE:
    case VK_CAPITAL:
    case VK_NUMLOCK:
    case VK_SCROLL:
    case VK_PAUSE:
    case VK_SNAPSHOT:
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
        return true;
    default:
        return false;
    }
}

LRESULT CALLBACK KeybindManager::LowLevelKeyboardProc(
    int code,
    WPARAM wparam,
    LPARAM lparam)
{
    if (code == HC_ACTION && active_ && active_->capturing_) {
        if (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN) {
            const auto* info = reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam);
            if (!active_->IsArmed()) {
                return CallNextHookEx(active_->hook_, code, wparam, lparam);
            }

            const bool extended =
                (info->flags & LLKHF_EXTENDED) != 0;
            const int vk = NormalizeVirtualKey(
                static_cast<int>(info->vkCode),
                extended);

            if (vk == VK_ESCAPE) {
                active_->pending_cancel_ = true;
                if (active_->notify_hwnd_) {
                    PostMessageW(
                        active_->notify_hwnd_,
                        KeybindManager::WM_KEYBIND,
                        0,
                        0);
                }
                return CallNextHookEx(active_->hook_, code, wparam, lparam);
            }

            if (IsAllowedVirtualKey(vk)) {
                active_->pending_vk_ = vk;
                if (active_->notify_hwnd_) {
                    PostMessageW(
                        active_->notify_hwnd_,
                        KeybindManager::WM_KEYBIND,
                        0,
                        0);
                }
            }
        }
    }

    if (active_ && active_->hook_) {
        return CallNextHookEx(active_->hook_, code, wparam, lparam);
    }
    return CallNextHookEx(nullptr, code, wparam, lparam);
}

bool KeybindManager::PollAndApply(KeybindConfig* config)
{
    if (!capturing_ || !config) {
        return false;
    }

    if (pending_cancel_) {
        CancelCapture();
        return true;
    }

    if (pending_vk_ == 0) {
        return false;
    }

    const int vk = pending_vk_;
    pending_vk_ = 0;
    return ApplyVirtualKey(vk, config);
}

bool KeybindManager::ApplyVirtualKey(int vk, KeybindConfig* config)
{
    if (!capturing_ || !config) {
        return false;
    }

    if (!IsArmed()) {
        return false;
    }

    if (vk == VK_ESCAPE) {
        CancelCapture();
        return true;
    }

    if (!IsAllowedVirtualKey(vk)) {
        return false;
    }

    if (slot_ == KeybindSlot::RecordStop) {
        config->record_vk = vk;
    } else {
        config->play_vk = vk;
    }

    capturing_ = false;
    arm_at_ms_ = 0;
    pending_vk_ = 0;
    pending_cancel_ = false;
    RemoveHook();
    DrainAsyncKeyState();
    return true;
}

std::wstring KeybindManager::VirtualKeyToLabel(int vk)
{
    if (vk == VK_NUMENTER) {
        return L"Num Enter";
    }

    if (vk >= '0' && vk <= '9') {
        return std::wstring(1, static_cast<wchar_t>(vk));
    }
    if (vk >= 'A' && vk <= 'Z') {
        return std::wstring(1, static_cast<wchar_t>(vk));
    }

    if (vk >= VK_F1 && vk <= VK_F24) {
        wchar_t buffer[8] = {};
        wsprintfW(buffer, L"F%d", vk - VK_F1 + 1);
        return buffer;
    }

    switch (vk) {
    case VK_NUMPAD0: return L"Num 0";
    case VK_NUMPAD1: return L"Num 1";
    case VK_NUMPAD2: return L"Num 2";
    case VK_NUMPAD3: return L"Num 3";
    case VK_NUMPAD4: return L"Num 4";
    case VK_NUMPAD5: return L"Num 5";
    case VK_NUMPAD6: return L"Num 6";
    case VK_NUMPAD7: return L"Num 7";
    case VK_NUMPAD8: return L"Num 8";
    case VK_NUMPAD9: return L"Num 9";
    case VK_MULTIPLY: return L"Num *";
    case VK_ADD: return L"Num +";
    case VK_SEPARATOR: return L"Num Sep";
    case VK_SUBTRACT: return L"Num -";
    case VK_DECIMAL: return L"Num .";
    case VK_DIVIDE: return L"Num /";
    case VK_BACK: return L"Backspace";
    case VK_TAB: return L"Tab";
    case VK_RETURN: return L"Enter";
    case VK_SPACE: return L"Space";
    case VK_CAPITAL: return L"CapsLock";
    case VK_NUMLOCK: return L"NumLock";
    case VK_SCROLL: return L"ScrollLock";
    case VK_PAUSE: return L"Pause";
    case VK_SNAPSHOT: return L"PrintScreen";
    case VK_INSERT: return L"Insert";
    case VK_DELETE: return L"Delete";
    case VK_HOME: return L"Home";
    case VK_END: return L"End";
    case VK_PRIOR: return L"PageUp";
    case VK_NEXT: return L"PageDown";
    case VK_LEFT: return L"Left";
    case VK_UP: return L"Up";
    case VK_RIGHT: return L"Right";
    case VK_DOWN: return L"Down";
    case VK_ESCAPE: return L"Esc";
    default:
        break;
    }

    wchar_t fallback[32] = {};
    wsprintfW(fallback, L"VK_%02X", vk);
    return fallback;
}
