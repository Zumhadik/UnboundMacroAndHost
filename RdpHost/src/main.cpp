#include "rdphost/AppController.h"
#include "rdphost/Paths.h"
#include "rdphost/ui/MainWindow.h"

#include <windows.h>

#include <memory>
#include <string>

namespace {

std::wstring GetExecutableDirectory() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD length =
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return {};
    }

    std::wstring path(buffer, length);
    const size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return {};
    }
    return path.substr(0, pos);
}
}
int WINAPI wWinMain(HINSTANCE instance,
                    HINSTANCE,
                    PWSTR,
                    int) {
    const rdphost::AppPaths paths =
        rdphost::Paths::Resolve(GetExecutableDirectory());

    auto controller = std::make_shared<rdphost::AppController>(paths);
    rdphost::ui::MainWindow window(controller);

    if (!window.Create(instance)) {
        MessageBoxW(
            nullptr,
            L"Failed to create main window.",
            L"RdpHost",
            MB_ICONERROR | MB_OK);
        return 1;
    }

    return window.RunMessageLoop();
}