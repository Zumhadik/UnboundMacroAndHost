#include "rdphost/ui/UiColors.h"

namespace rdphost::ui {

COLORREF ColorForStatus(StatusLevel level) {
    switch (level) {
        case StatusLevel::Ok:
            return RGB(0, 140, 0);
        case StatusLevel::Warn:
            return RGB(170, 110, 0);
        case StatusLevel::Error:
            return RGB(180, 0, 0);
        case StatusLevel::Unknown:
            return RGB(90, 90, 90);
    }

    return RGB(90, 90, 90);
}
const wchar_t* TextForStatus(StatusLevel level) {
    switch (level) {
        case StatusLevel::Ok:
            return L"OK";
        case StatusLevel::Warn:
            return L"WARN";
        case StatusLevel::Error:
            return L"FAIL";
        case StatusLevel::Unknown:
            return L"???";
    }

    return L"???";
}
}