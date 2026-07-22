#pragma once

#include "../core/Types.h"
#include "../keybinds/KeybindManager.h"
#include "../playback/MacroPlayer.h"
#include "../recording/InputHook.h"
#include "../recording/MacroRecorder.h"
#include "Controls.h"
#include "FontLoader.h"
#include "StatusPanel.h"

#include <windows.h>

class MainWindow {
public:
    explicit MainWindow(HINSTANCE instance);
    ~MainWindow();

    bool Create();
    int RunMessageLoop();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

    bool BuildUi();
    void UpdateLabels();
    void UpdateLoopBtn();
    void RefreshUi();
    void InvalidateStatusArea();
    RECT StatusPanelRect() const;
    void SyncHotkeys();
    void PaintWindow(HDC hdc, const RECT& client) const;

    void OnCreateFile();
    void OnOpenFile();
    void OnLoopToggle();
    void OnKeybindRecord();
    void OnKeybindPlay();
    void OnKeybindNotify();
    void OnToggleRecord();
    void OnTogglePlay();
    void OnPlayFinished();
    void OnUiTick();

    void StopRecording();
    void StopPlaying();
    bool CheckFile();

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    FontLoader font_loader_;
    HFONT ui_font_ = nullptr;
    HFONT title_font_ = nullptr;

    StyledButton btn_create_;
    StyledButton btn_open_;
    StyledButton btn_loop_;
    StyledButton btn_key_record_;
    StyledButton btn_key_play_;
    StatusPanel status_panel_;

    AppState state_{};
    KeybindManager keybinds_;
    InputHook input_hook_;
    MacroRecorder recorder_;
    MacroPlayer player_;
    std::uint64_t last_hotkey_ms_ = 0;
    std::uint64_t last_drawn_sec_ = 0xFFFFFFFFFFFFFFFFULL;
    std::uint64_t macro_duration_ms_ = 0;
};
