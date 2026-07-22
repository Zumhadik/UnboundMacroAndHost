#pragma once

#include <windows.h>

#define APP_TITLE L"UnboundMacro"
#define APP_WNDCLASS L"UnboundMacroWindow"
#define MACRO_EXT L"macros"
#define MACRO_FILTER L"Macro Files (*.macros)\0*.macros\0All Files (*.*)\0*.*\0"
#define APP_WIDTH 420
#define APP_HEIGHT 356
#define IDC_CREATE 1001
#define IDC_OPEN 1002
#define IDC_LOOP 1004
#define IDC_KEY_RECORD 1005
#define IDC_KEY_PLAY 1006
#define IDT_UI 1
#define UI_TICK_MS 16
#define WM_TOGGLE_RECORD (WM_APP + 41)
#define WM_TOGGLE_PLAY (WM_APP + 42)
#define WM_PLAY_FINISHED (WM_APP + 43)
