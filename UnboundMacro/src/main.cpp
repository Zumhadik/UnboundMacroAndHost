#include "app/Application.h"

#include <windows.h>

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int)
{
    return Application(instance).Run();
}
