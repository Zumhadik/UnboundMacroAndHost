#include "Application.h"

#include "../ui/MainWindow.h"

#include <timeapi.h>

Application::Application(HINSTANCE instance)
    : instance_(instance)
{
}

int Application::Run()
{
    timeBeginPeriod(1);

    MainWindow window(instance_);
    if (!window.Create()) {
        timeEndPeriod(1);
        MessageBoxW(
            nullptr,
            L"Failed to create main window.",
            L"UnboundMacro",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    const int code = window.RunMessageLoop();
    timeEndPeriod(1);
    return code;
}
