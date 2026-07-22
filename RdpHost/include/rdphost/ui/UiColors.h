#pragma once

#include "rdphost/Types.h"

#include <windows.h>

namespace rdphost::ui {

COLORREF ColorForStatus(StatusLevel level);
const wchar_t* TextForStatus(StatusLevel level);

}