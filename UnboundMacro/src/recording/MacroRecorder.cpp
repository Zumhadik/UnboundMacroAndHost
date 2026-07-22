#include "MacroRecorder.h"

#include "../core/Clock.h"
#include "../macro/MacroFormat.h"

#include <windows.h>

namespace {
constexpr std::uint64_t MOVE_MIN_MS = 8;
}

bool MacroRecorder::Start(const std::wstring& path)
{
    if (recording_) {
        Stop();
    }

    path_ = path;
    events_.clear();
    events_.reserve(8192);
    start_ms_ = Clock::NowMs();
    last_event_ms_ = start_ms_;
    last_move_ms_ = 0;
    last_duration_ms_ = 0;
    has_pos_ = false;
    recording_ = true;
    return true;
}

bool MacroRecorder::Stop()
{
    if (!recording_) {
        return false;
    }

    last_duration_ms_ = Clock::NowMs() - start_ms_;
    recording_ = false;
    const std::uint32_t duration = last_duration_ms_ > 0xFFFFFFFFULL
        ? 0xFFFFFFFFU
        : static_cast<std::uint32_t>(last_duration_ms_);
    return MacroFormat::Save(path_, events_, duration);
}

bool MacroRecorder::IsRecording() const
{
    return recording_;
}

std::size_t MacroRecorder::EventCount() const
{
    return events_.size();
}

std::uint64_t MacroRecorder::ElapsedMs() const
{
    if (!recording_) {
        return last_duration_ms_;
    }
    return Clock::NowMs() - start_ms_;
}

std::uint64_t MacroRecorder::LastDurationMs() const
{
    return last_duration_ms_;
}

std::uint32_t MacroRecorder::TakeDeltaMs()
{
    const std::uint64_t now = Clock::NowMs();
    std::uint64_t dt = 0;
    if (now >= last_event_ms_) {
        dt = now - last_event_ms_;
    }
    last_event_ms_ = now;
    if (dt > 0xFFFFFFFFULL) {
        dt = 0xFFFFFFFFULL;
    }
    return static_cast<std::uint32_t>(dt);
}

void MacroRecorder::Push(MacroEvent event)
{
    if (!recording_) {
        return;
    }
    event.dt_ms = TakeDeltaMs();
    events_.push_back(event);
}

void MacroRecorder::OnMouseMove(int x, int y)
{
    if (!recording_) {
        return;
    }

    if (has_pos_ && x == last_x_ && y == last_y_) {
        return;
    }

    const std::uint64_t now = Clock::NowMs();
    if (has_pos_ && last_move_ms_ != 0 &&
        now - last_move_ms_ < MOVE_MIN_MS) {
        last_x_ = x;
        last_y_ = y;
        return;
    }

    MacroEvent event;
    event.type = MacroEventType::Move;
    event.x = x;
    event.y = y;
    Push(event);

    last_x_ = x;
    last_y_ = y;
    has_pos_ = true;
    last_move_ms_ = now;
}

void MacroRecorder::OnMouseButton(int button, bool down, int x, int y)
{
    if (!recording_) {
        return;
    }

    if (!has_pos_ || x != last_x_ || y != last_y_) {
        MacroEvent move;
        move.type = MacroEventType::Move;
        move.x = x;
        move.y = y;
        Push(move);
        last_x_ = x;
        last_y_ = y;
        has_pos_ = true;
        last_move_ms_ = Clock::NowMs();
    }

    MacroEvent event;
    event.type = down ? MacroEventType::BtnDown : MacroEventType::BtnUp;
    event.button = static_cast<std::uint8_t>(button);
    event.x = x;
    event.y = y;
    Push(event);
}

void MacroRecorder::OnWheel(int notches, int x, int y)
{
    if (!recording_ || notches == 0) {
        return;
    }

    if (!has_pos_ || x != last_x_ || y != last_y_) {
        MacroEvent move;
        move.type = MacroEventType::Move;
        move.x = x;
        move.y = y;
        Push(move);
        last_x_ = x;
        last_y_ = y;
        has_pos_ = true;
    }

    MacroEvent event;
    event.type = MacroEventType::Wheel;
    event.wheel = static_cast<std::int16_t>(notches);
    event.x = x;
    event.y = y;
    Push(event);
}

void MacroRecorder::OnKey(int vk, bool down)
{
    if (!recording_) {
        return;
    }

    MacroEvent event;
    event.type = down ? MacroEventType::KeyDown : MacroEventType::KeyUp;
    event.vk = static_cast<std::uint16_t>(vk);
    Push(event);
}
