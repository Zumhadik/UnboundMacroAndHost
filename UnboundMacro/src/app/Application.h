#pragma once

#include <windows.h>

class Application {
public:
    explicit Application(HINSTANCE instance);
    int Run();

private:
    HINSTANCE instance_ = nullptr;
};
