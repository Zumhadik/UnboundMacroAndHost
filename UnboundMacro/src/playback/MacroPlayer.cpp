#include "MacroPlayer.h"

#include "../core/Clock.h"
#include "../core/Constants.h"
#include "../keybinds/KeybindManager.h"
#include "../macro/MacroFormat.h"

#ifndef MOUSEEVENTF_MOVE_NOCOALESCE
#define MOUSEEVENTF_MOVE_NOCOALESCE 0x2000
#endif

namespace {
constexpr int MAX_REL_STEP = 100;

int ClampStep(int value)
{
    if (value > MAX_REL_STEP) {
        return MAX_REL_STEP;
    }
    if (value < -MAX_REL_STEP) {
        return -MAX_REL_STEP;
    }
    return value;
}
}

MacroPlayer::~MacroPlayer()
{
    Stop();
}

bool MacroPlayer::Load(const std::wstring& path)
{
    if (!MacroFormat::Load(path, &events_, nullptr)) {
        return false;
    }

    absolute_ms_.clear();
    absolute_ms_.reserve(events_.size());
    std::uint64_t cursor = 0;
    for (const MacroEvent& event : events_) {
        cursor += event.dt_ms;
        absolute_ms_.push_back(cursor);
    }
    return true;
}

bool MacroPlayer::Start(HWND notify_hwnd, bool loop)
{
    Stop();
    if (events_.empty()) {
        return false;
    }

    notify_hwnd_ = notify_hwnd;
    loop_ = loop;
    stop_requested_ = false;
    playing_ = true;
    start_ms_ = Clock::NowMs();
    worker_ = std::thread(&MacroPlayer::ThreadMain, this);
    return true;
}

void MacroPlayer::Stop()
{
    stop_requested_ = true;
    if (worker_.joinable()) {
        worker_.join();
    }
    EndInputProfile();
    playing_ = false;
    stop_requested_ = false;
}

bool MacroPlayer::IsPlaying() const
{
    return playing_;
}

std::uint64_t MacroPlayer::ElapsedMs() const
{
    if (!playing_) {
        return 0;
    }
    const std::uint64_t now = Clock::NowMs();
    return now > start_ms_ ? now - start_ms_ : 0;
}

void MacroPlayer::ResetCursorState()
{
    cursor_ready_ = false;
    cursor_x_ = 0;
    cursor_y_ = 0;
}

void MacroPlayer::BeginInputProfile()
{
    saved_mouse_ = false;
    if (SystemParametersInfoW(SPI_GETMOUSE, 0, saved_mouse_params_, 0)) {
        int flat[3] = {0, 0, 0};
        if (SystemParametersInfoW(SPI_SETMOUSE, 0, flat, 0)) {
            saved_mouse_ = true;
        }
    }

    saved_mouse_speed_ = 10;
    SystemParametersInfoW(SPI_GETMOUSESPEED, 0, &saved_mouse_speed_, 0);
    constexpr int linear_speed = 10;
    SystemParametersInfoW(
        SPI_SETMOUSESPEED,
        0,
        reinterpret_cast<void*>(static_cast<INT_PTR>(linear_speed)),
        0);
}

void MacroPlayer::EndInputProfile()
{
    if (saved_mouse_) {
        SystemParametersInfoW(
            SPI_SETMOUSE,
            0,
            saved_mouse_params_,
            SPIF_SENDCHANGE);
        saved_mouse_ = false;
    }

    SystemParametersInfoW(
        SPI_SETMOUSESPEED,
        0,
        reinterpret_cast<void*>(static_cast<INT_PTR>(saved_mouse_speed_)),
        SPIF_SENDCHANGE);
}

bool MacroPlayer::WaitUntilMs(
    std::uint64_t start_ms,
    std::uint64_t target_ms)
{
    for (;;) {
        if (stop_requested_) {
            return false;
        }

        const std::uint64_t elapsed = Clock::NowMs() - start_ms;
        if (elapsed >= target_ms) {
            return true;
        }

        if (target_ms - elapsed > 2) {
            Sleep(1);
        }
    }
}

bool MacroPlayer::WaitMoveInterpolated(
    std::uint64_t start_ms,
    std::uint64_t from_abs_ms,
    std::uint64_t target_ms,
    int from_x,
    int from_y,
    int to_x,
    int to_y)
{
    if (from_x == to_x && from_y == to_y) {
        return WaitUntilMs(start_ms, target_ms);
    }

    const std::uint64_t span =
        target_ms > from_abs_ms ? target_ms - from_abs_ms : 0;
    int prev_x = from_x;
    int prev_y = from_y;

    while (!stop_requested_) {
        const std::uint64_t elapsed = Clock::NowMs() - start_ms;
        if (elapsed >= target_ms) {
            break;
        }

        double t = 1.0;
        if (span > 0 && elapsed > from_abs_ms) {
            t = static_cast<double>(elapsed - from_abs_ms) /
                static_cast<double>(span);
        } else if (elapsed <= from_abs_ms) {
            t = 0.0;
        }

        if (t < 0.0) {
            t = 0.0;
        }
        if (t > 1.0) {
            t = 1.0;
        }

        const int x = static_cast<int>(from_x + (to_x - from_x) * t);
        const int y = static_cast<int>(from_y + (to_y - from_y) * t);
        if (x != prev_x || y != prev_y) {
            MoveTo(x, y);
            prev_x = x;
            prev_y = y;
        }

        if (target_ms - elapsed > 2) {
            Sleep(1);
        }
    }

    if (stop_requested_) {
        return false;
    }

    if (prev_x != to_x || prev_y != to_y) {
        MoveTo(to_x, to_y);
    }
    return true;
}

void MacroPlayer::ThreadMain()
{
    BeginInputProfile();

    do {
        ResetCursorState();
        Sleep(30);
        start_ms_ = Clock::NowMs();
        int last_x = 0;
        int last_y = 0;
        bool has_pos = false;
        std::uint64_t prev_abs_ms = 0;

        for (std::size_t i = 0; i < events_.size(); ++i) {
            if (stop_requested_) {
                EndInputProfile();
                playing_ = false;
                return;
            }

            const MacroEvent& event = events_[i];
            const std::uint64_t target_ms = absolute_ms_[i];

            if (event.type == MacroEventType::Move && has_pos) {
                if (!WaitMoveInterpolated(
                        start_ms_,
                        prev_abs_ms,
                        target_ms,
                        last_x,
                        last_y,
                        event.x,
                        event.y)) {
                    EndInputProfile();
                    playing_ = false;
                    return;
                }
            } else {
                if (!WaitUntilMs(start_ms_, target_ms)) {
                    EndInputProfile();
                    playing_ = false;
                    return;
                }
                PlayEvent(event);
            }

            if (event.type == MacroEventType::Move ||
                event.type == MacroEventType::BtnDown ||
                event.type == MacroEventType::BtnUp ||
                event.type == MacroEventType::Wheel) {
                last_x = event.x;
                last_y = event.y;
                has_pos = true;
            }

            prev_abs_ms = target_ms;
        }
    } while (loop_ && !stop_requested_);

    EndInputProfile();
    playing_ = false;
    if (notify_hwnd_ && !stop_requested_) {
        PostMessageW(notify_hwnd_, WM_PLAY_FINISHED, 0, 0);
    }
}

void MacroPlayer::PlayEvent(const MacroEvent& event)
{
    switch (event.type) {
    case MacroEventType::Move:
        MoveTo(event.x, event.y);
        break;
    case MacroEventType::BtnDown:
        MoveTo(event.x, event.y);
        SendMouseButton(event.button, true);
        break;
    case MacroEventType::BtnUp:
        MoveTo(event.x, event.y);
        SendMouseButton(event.button, false);
        break;
    case MacroEventType::Wheel:
        MoveTo(event.x, event.y);
        SendWheel(event.wheel);
        break;
    case MacroEventType::KeyDown:
        SendKey(event.vk, true);
        break;
    case MacroEventType::KeyUp:
        SendKey(event.vk, false);
        break;
    default:
        break;
    }
}

void MacroPlayer::MoveTo(int x, int y)
{
    if (cursor_ready_) {
        const int dx = x - cursor_x_;
        const int dy = y - cursor_y_;
        if (dx != 0 || dy != 0) {
            SendMouseRelative(dx, dy);
        }
    }

    SetCursorPos(x, y);
    SendMouseAbsolute(x, y);

    POINT real = {};
    if (GetCursorPos(&real)) {
        cursor_x_ = real.x;
        cursor_y_ = real.y;
    } else {
        cursor_x_ = x;
        cursor_y_ = y;
    }
    cursor_ready_ = true;

    if (cursor_x_ != x || cursor_y_ != y) {
        const int fix_x = x - cursor_x_;
        const int fix_y = y - cursor_y_;
        if (fix_x != 0 || fix_y != 0) {
            SendMouseRelative(fix_x, fix_y);
        }
        SetCursorPos(x, y);
        SendMouseAbsolute(x, y);
        cursor_x_ = x;
        cursor_y_ = y;
    }
}

void MacroPlayer::SendMouseAbsolute(int x, int y)
{
    const int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (vw <= 1 || vh <= 1) {
        return;
    }

    if (x < vx) {
        x = vx;
    }
    if (y < vy) {
        y = vy;
    }
    if (x > vx + vw - 1) {
        x = vx + vw - 1;
    }
    if (y > vy + vh - 1) {
        y = vy + vh - 1;
    }

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<LONG>(
        (static_cast<long long>(x - vx) * 65535LL) / (vw - 1));
    input.mi.dy = static_cast<LONG>(
        (static_cast<long long>(y - vy) * 65535LL) / (vh - 1));
    if (input.mi.dx < 0) {
        input.mi.dx = 0;
    }
    if (input.mi.dy < 0) {
        input.mi.dy = 0;
    }
    if (input.mi.dx > 65535) {
        input.mi.dx = 65535;
    }
    if (input.mi.dy > 65535) {
        input.mi.dy = 65535;
    }
    input.mi.dwFlags =
        MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    SendInput(1, &input, sizeof(INPUT));
}

void MacroPlayer::SendMouseRelative(int dx, int dy)
{
    int left_x = dx;
    int left_y = dy;

    while (left_x != 0 || left_y != 0) {
        const int step_x = ClampStep(left_x);
        const int step_y = ClampStep(left_y);

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = step_x;
        input.mi.dy = step_y;
        input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_MOVE_NOCOALESCE;
        SendInput(1, &input, sizeof(INPUT));

        left_x -= step_x;
        left_y -= step_y;
    }
}

void MacroPlayer::SendMouseButton(int button, bool down)
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    switch (button) {
    case 1:
        input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        break;
    case 2:
        input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        break;
    case 3:
        input.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        break;
    default:
        return;
    }
    SendInput(1, &input, sizeof(INPUT));
}

void MacroPlayer::SendWheel(int notches)
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL | MOUSEEVENTF_MOVE_NOCOALESCE;
    input.mi.mouseData = static_cast<DWORD>(notches * WHEEL_DELTA);
    SendInput(1, &input, sizeof(INPUT));
}

bool MacroPlayer::KeyNeedsExtended(int vk)
{
    if (vk == VK_NUMENTER) {
        return true;
    }

    switch (vk) {
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_DIVIDE:
    case VK_NUMLOCK:
    case VK_RCONTROL:
    case VK_RMENU:
        return true;
    default:
        return false;
    }
}

WORD MacroPlayer::VkToScan(int vk, bool* extended)
{
    int use_vk = vk;
    if (vk == VK_NUMENTER) {
        use_vk = VK_RETURN;
        if (extended) {
            *extended = true;
        }
    } else if (extended) {
        *extended = KeyNeedsExtended(vk);
    }

    const UINT mapped = MapVirtualKeyW(
        static_cast<UINT>(use_vk),
        MAPVK_VK_TO_VSC_EX);
    WORD scan = static_cast<WORD>(mapped & 0xFF);
    if ((mapped & 0xFF00) != 0 && extended) {
        *extended = true;
    }

    if (scan == 0) {
        scan = static_cast<WORD>(MapVirtualKeyW(
            static_cast<UINT>(use_vk),
            MAPVK_VK_TO_VSC));
    }
    return scan;
}

void MacroPlayer::SendKey(int vk, bool down)
{
    bool extended = false;
    const WORD scan = VkToScan(vk, &extended);

    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = scan;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (!down) {
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    }
    if (extended) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    SendInput(1, &input, sizeof(INPUT));
}
