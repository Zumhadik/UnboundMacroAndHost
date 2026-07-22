#include "MacroFormat.h"

#include <cstdio>
#include <windows.h>

namespace {
constexpr char MAGIC_V2[4] = {'U', 'M', '1', 2};
constexpr char MAGIC_V1[4] = {'U', 'M', '1', 1};

class WideFile {
public:
    WideFile(const std::wstring& path, const wchar_t* mode)
    {
        if (_wfopen_s(&file_, path.c_str(), mode) != 0) {
            file_ = nullptr;
        }
    }

    ~WideFile()
    {
        if (file_) {
            fclose(file_);
        }
    }

    bool Ok() const
    {
        return file_ != nullptr;
    }

    bool Write(const void* data, std::size_t size)
    {
        return fwrite(data, 1, size, file_) == size;
    }

    bool Read(void* data, std::size_t size)
    {
        return fread(data, 1, size, file_) == size;
    }

private:
    FILE* file_ = nullptr;
};

template <typename T>
bool WritePod(WideFile& out, const T& value)
{
    return out.Write(&value, sizeof(T));
}

template <typename T>
bool ReadPod(WideFile& in, T* value)
{
    return in.Read(value, sizeof(T));
}

std::uint32_t SumEventDuration(const MacroEvents& events)
{
    std::uint64_t total = 0;
    for (const MacroEvent& event : events) {
        total += event.dt_ms;
    }
    if (total > 0xFFFFFFFFULL) {
        total = 0xFFFFFFFFULL;
    }
    return static_cast<std::uint32_t>(total);
}

bool ReadEvents(
    WideFile& in,
    MacroEvents* events,
    std::uint32_t count)
{
    events->clear();
    events->reserve(count);

    for (std::uint32_t i = 0; i < count; ++i) {
        MacroEvent event;
        std::uint8_t type = 0;
        if (!ReadPod(in, &event.dt_ms) || !ReadPod(in, &type)) {
            return false;
        }
        event.type = static_cast<MacroEventType>(type);

        switch (event.type) {
        case MacroEventType::Move:
            if (!ReadPod(in, &event.x) || !ReadPod(in, &event.y)) {
                return false;
            }
            break;
        case MacroEventType::BtnDown:
        case MacroEventType::BtnUp:
            if (!ReadPod(in, &event.button) ||
                !ReadPod(in, &event.x) ||
                !ReadPod(in, &event.y)) {
                return false;
            }
            break;
        case MacroEventType::Wheel:
            if (!ReadPod(in, &event.wheel) ||
                !ReadPod(in, &event.x) ||
                !ReadPod(in, &event.y)) {
                return false;
            }
            break;
        case MacroEventType::KeyDown:
        case MacroEventType::KeyUp:
            if (!ReadPod(in, &event.vk)) {
                return false;
            }
            break;
        default:
            return false;
        }

        events->push_back(event);
    }

    return true;
}
}

bool MacroFormat::WriteEmpty(const std::wstring& path)
{
    return Save(path, {}, 0);
}

bool MacroFormat::Save(
    const std::wstring& path,
    const MacroEvents& events,
    std::uint32_t duration_ms)
{
    WideFile out(path, L"wb");
    if (!out.Ok()) {
        return false;
    }

    if (!out.Write(MAGIC_V2, 4)) {
        return false;
    }

    if (!WritePod(out, duration_ms)) {
        return false;
    }

    const std::uint32_t count = static_cast<std::uint32_t>(events.size());
    if (!WritePod(out, count)) {
        return false;
    }

    for (const MacroEvent& event : events) {
        if (!WritePod(out, event.dt_ms)) {
            return false;
        }
        const std::uint8_t type = static_cast<std::uint8_t>(event.type);
        if (!WritePod(out, type)) {
            return false;
        }

        switch (event.type) {
        case MacroEventType::Move:
            if (!WritePod(out, event.x) || !WritePod(out, event.y)) {
                return false;
            }
            break;
        case MacroEventType::BtnDown:
        case MacroEventType::BtnUp:
            if (!WritePod(out, event.button) ||
                !WritePod(out, event.x) ||
                !WritePod(out, event.y)) {
                return false;
            }
            break;
        case MacroEventType::Wheel:
            if (!WritePod(out, event.wheel) ||
                !WritePod(out, event.x) ||
                !WritePod(out, event.y)) {
                return false;
            }
            break;
        case MacroEventType::KeyDown:
        case MacroEventType::KeyUp:
            if (!WritePod(out, event.vk)) {
                return false;
            }
            break;
        default:
            return false;
        }
    }

    return true;
}

bool MacroFormat::Load(
    const std::wstring& path,
    MacroEvents* events,
    std::uint32_t* duration_ms)
{
    if (!events) {
        return false;
    }

    events->clear();
    WideFile in(path, L"rb");
    if (!in.Ok()) {
        return false;
    }

    char magic[4] = {};
    if (!in.Read(magic, 4) ||
        magic[0] != 'U' ||
        magic[1] != 'M' ||
        magic[2] != '1') {
        return false;
    }

    std::uint32_t stored_duration = 0;
    if (magic[3] == MAGIC_V2[3]) {
        if (!ReadPod(in, &stored_duration)) {
            return false;
        }
    } else if (magic[3] != MAGIC_V1[3]) {
        return false;
    }

    std::uint32_t count = 0;
    if (!ReadPod(in, &count)) {
        return false;
    }

    if (!ReadEvents(in, events, count)) {
        return false;
    }

    if (duration_ms) {
        if (magic[3] == MAGIC_V2[3]) {
            *duration_ms = stored_duration;
        } else {
            *duration_ms = SumEventDuration(*events);
        }
    }

    return true;
}

bool MacroFormat::ReadDuration(
    const std::wstring& path,
    std::uint32_t* duration_ms)
{
    MacroEvents events;
    return Load(path, &events, duration_ms);
}
