#include "MainWindow.h"

#include "../core/Constants.h"
#include "../macro/MacroFile.h"
#include "../macro/MacroFormat.h"
#include "Theme.h"

#include "resource.h"

namespace {
RECT MakeRect(int x, int y, int w, int h)
{
    RECT rc = {x, y, x + w, y + h};
    return rc;
}

constexpr std::uint64_t HOTKEY_DEBOUNCE_MS = 250;
}

MainWindow::MainWindow(HINSTANCE instance)
    : instance_(instance)
{
}

MainWindow::~MainWindow()
{
    StopRecording();
    StopPlaying();
    keybinds_.CancelCapture();
    input_hook_.Uninstall();
    if (ui_font_) {
        DeleteObject(ui_font_);
    }
    if (title_font_) {
        DeleteObject(title_font_);
    }
    font_loader_.Unload();
    Theme::Shutdown();
}

bool MainWindow::Create()
{
    font_loader_.LoadFromResource(instance_, IDR_FONT_SFPRO);
    ui_font_ = font_loader_.CreateUiFont(15);
    title_font_ = font_loader_.CreateUiFont(20);
    if (!ui_font_) {
        ui_font_ = CreateFontW(
            -15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    }
    if (!title_font_) {
        title_font_ = CreateFontW(
            -20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    }

    status_panel_.SetFont(ui_font_);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance_;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = Theme::BackgroundBrush();
    wc.lpszClassName = APP_WNDCLASS;
    wc.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APPICON));
    if (!wc.hIcon) {
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    wc.hIconSm = wc.hIcon;

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = {0, 0, APP_WIDTH, APP_HEIGHT};
    AdjustWindowRect(&rc, style, FALSE);

    hwnd_ = CreateWindowExW(
        0,
        APP_WNDCLASS,
        APP_TITLE,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        nullptr,
        nullptr,
        instance_,
        this);

    if (!hwnd_) {
        return false;
    }

    if (!BuildUi()) {
        return false;
    }

    input_hook_.SetRecorder(&recorder_);
    if (!input_hook_.Install(hwnd_)) {
        MessageBoxW(
            hwnd_,
            L"Failed to install input hooks.",
            APP_TITLE,
            MB_OK | MB_ICONERROR);
        return false;
    }
    SyncHotkeys();

    UpdateLabels();
    UpdateLoopBtn();
    RefreshUi();

    SetTimer(hwnd_, IDT_UI, UI_TICK_MS, nullptr);
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return true;
}

int MainWindow::RunMessageLoop()
{
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

bool MainWindow::BuildUi()
{
    const int margin = 20;
    const int gap = 10;
    const int btn_h = 32;
    const int btn_w = 180;

    if (!btn_create_.Create(
            hwnd_,
            IDC_CREATE,
            MakeRect(margin, 50, btn_w, btn_h),
            L"Create",
            ui_font_)) {
        return false;
    }
    if (!btn_open_.Create(
            hwnd_,
            IDC_OPEN,
            MakeRect(margin + btn_w + gap, 50, btn_w, btn_h),
            L"Open",
            ui_font_)) {
        return false;
    }
    if (!btn_loop_.Create(
            hwnd_,
            IDC_LOOP,
            MakeRect(margin, 90, 380, btn_h),
            L"Loop: Off",
            ui_font_)) {
        return false;
    }
    if (!btn_key_record_.Create(
            hwnd_,
            IDC_KEY_RECORD,
            MakeRect(margin, 130, 380, btn_h),
            L"Keybind Record/Stop: F8",
            ui_font_)) {
        return false;
    }
    if (!btn_key_play_.Create(
            hwnd_,
            IDC_KEY_PLAY,
            MakeRect(margin, 170, 380, btn_h),
            L"Keybind Start/Stop: F10",
            ui_font_)) {
        return false;
    }

    return true;
}

void MainWindow::UpdateLabels()
{
    if (keybinds_.IsCapturing() &&
        keybinds_.CapturingSlot() == KeybindSlot::RecordStop) {
        btn_key_record_.SetText(L"Keybind Record/Stop: ...");
    } else {
        btn_key_record_.SetText(
            L"Keybind Record/Stop: " +
            KeybindManager::VirtualKeyToLabel(state_.keybinds.record_vk));
    }

    if (keybinds_.IsCapturing() &&
        keybinds_.CapturingSlot() == KeybindSlot::PlayStop) {
        btn_key_play_.SetText(L"Keybind Start/Stop: ...");
    } else {
        btn_key_play_.SetText(
            L"Keybind Start/Stop: " +
            KeybindManager::VirtualKeyToLabel(state_.keybinds.play_vk));
    }

    btn_loop_.SetText(state_.loop_enabled ? L"Loop: On" : L"Loop: Off");
}

void MainWindow::UpdateLoopBtn()
{
    ButtonStyle style = {};
    if (state_.loop_enabled) {
        style.face = RGB(34, 52, 44);
        style.face_hover = RGB(42, 66, 54);
        style.text = Theme::Colors().text;
        style.border = RGB(70, 110, 88);
    } else {
        style.face = Theme::Colors().button_face;
        style.face_hover = Theme::Colors().button_hover;
        style.text = Theme::Colors().text;
        style.border = Theme::Colors().border;
    }
    btn_loop_.SetStyle(style);
}

void MainWindow::SyncHotkeys()
{
    input_hook_.SetHotkeys(
        state_.keybinds.record_vk,
        state_.keybinds.play_vk);
}

RECT MainWindow::StatusPanelRect() const
{
    RECT client = {};
    GetClientRect(hwnd_, &client);
    RECT panel = {20, 234, client.right - 20, client.bottom - 12};
    return panel;
}

void MainWindow::InvalidateStatusArea()
{
    const RECT panel = StatusPanelRect();
    InvalidateRect(hwnd_, &panel, FALSE);
}

void MainWindow::RefreshUi()
{
    if (state_.is_recording) {
        state_.is_starting = false;
        state_.elapsed_ms = recorder_.ElapsedMs();
    } else if (state_.is_playing) {
        state_.is_starting = player_.IsStarting();
        state_.elapsed_ms = state_.is_starting
            ? player_.StartupLeftMs()
            : player_.ElapsedMs();
    } else {
        state_.is_starting = false;
    }

    last_drawn_sec_ = state_.elapsed_ms / 1000;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::PaintWindow(HDC hdc, const RECT& client) const
{
    FillRect(hdc, &client, Theme::BackgroundBrush());

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, Theme::Colors().text);
    HGDIOBJ old_font = SelectObject(hdc, title_font_);
    RECT title_rc = {20, 14, client.right - 20, 42};
    DrawTextW(
        hdc,
        APP_TITLE,
        -1,
        &title_rc,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    if (state_.has_file) {
        SelectObject(hdc, ui_font_);
        SetTextColor(hdc, Theme::Colors().muted);
        RECT path_rc = {20, 210, client.right - 20, 230};
        const std::wstring label = L"File: " + state_.macro_path;
        DrawTextW(
            hdc,
            label.c_str(),
            -1,
            &path_rc,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    const RECT panel = {20, 234, client.right - 20, client.bottom - 12};
    status_panel_.Paint(hdc, panel, state_);
    SelectObject(hdc, old_font);
}

bool MainWindow::CheckFile()
{
    if (state_.has_file) {
        return true;
    }

    MessageBoxW(
        hwnd_,
        L"Open or create a .macros file first.",
        APP_TITLE,
        MB_OK | MB_ICONINFORMATION);
    return false;
}

void MainWindow::StopRecording()
{
    if (!recorder_.IsRecording()) {
        state_.is_recording = false;
        input_hook_.SetCaptureEnabled(false);
        return;
    }

    input_hook_.SetCaptureEnabled(false);
    const std::size_t event_count = recorder_.EventCount();
    const bool saved = recorder_.Stop();
    macro_duration_ms_ = recorder_.LastDurationMs();
    state_.elapsed_ms = macro_duration_ms_;
    state_.is_recording = false;

    if (!saved) {
        MessageBoxW(
            hwnd_,
            L"Failed to save macro file.",
            APP_TITLE,
            MB_OK | MB_ICONERROR);
        return;
    }

    if (event_count == 0) {
        MessageBoxW(
            hwnd_,
            L"Recording saved, but no input was captured.",
            APP_TITLE,
            MB_OK | MB_ICONINFORMATION);
    }
}

void MainWindow::StopPlaying()
{
    if (player_.IsPlaying()) {
        player_.Stop();
    }
    state_.is_playing = false;
    state_.is_starting = false;
}

void MainWindow::OnCreateFile()
{
    StopRecording();
    StopPlaying();

    std::wstring path;
    if (!MacroFile::CreateNew(&path)) {
        return;
    }

    state_.macro_path = path;
    state_.has_file = true;
    macro_duration_ms_ = 0;
    state_.elapsed_ms = 0;
    keybinds_.CancelCapture();
    state_.waiting_keybind = false;
    input_hook_.SetHotkeysEnabled(true);
    UpdateLabels();
    RefreshUi();
}

void MainWindow::OnOpenFile()
{
    StopRecording();
    StopPlaying();

    std::wstring path;
    if (!MacroFile::OpenExisting(&path)) {
        return;
    }

    std::uint32_t duration_ms = 0;
    MacroEvents events;
    if (!MacroFormat::Load(path, &events, &duration_ms)) {
        MessageBoxW(
            hwnd_,
            L"Failed to read macro file.",
            APP_TITLE,
            MB_OK | MB_ICONERROR);
        return;
    }

    state_.macro_path = path;
    state_.has_file = true;
    macro_duration_ms_ = duration_ms;
    state_.elapsed_ms = duration_ms;
    keybinds_.CancelCapture();
    state_.waiting_keybind = false;
    input_hook_.SetHotkeysEnabled(true);
    UpdateLabels();
    RefreshUi();
}

void MainWindow::OnLoopToggle()
{
    state_.loop_enabled = !state_.loop_enabled;
    UpdateLabels();
    UpdateLoopBtn();
    RefreshUi();
}

void MainWindow::OnKeybindRecord()
{
    input_hook_.SetHotkeysEnabled(false);
    keybinds_.BeginCapture(KeybindSlot::RecordStop, hwnd_);
    state_.waiting_keybind = true;
    SetFocus(hwnd_);
    UpdateLabels();
    RefreshUi();
}

void MainWindow::OnKeybindPlay()
{
    input_hook_.SetHotkeysEnabled(false);
    keybinds_.BeginCapture(KeybindSlot::PlayStop, hwnd_);
    state_.waiting_keybind = true;
    SetFocus(hwnd_);
    UpdateLabels();
    RefreshUi();
}

void MainWindow::OnKeybindNotify()
{
    if (!keybinds_.IsCapturing()) {
        return;
    }

    if (keybinds_.PollAndApply(&state_.keybinds)) {
        state_.waiting_keybind = keybinds_.IsCapturing();
        if (!state_.waiting_keybind) {
            if (state_.keybinds.record_vk == state_.keybinds.play_vk) {
                state_.keybinds.play_vk =
                    (state_.keybinds.record_vk == VK_F10) ? VK_F9 : VK_F10;
            }
            input_hook_.SetHotkeysEnabled(true);
            SyncHotkeys();
        }
        UpdateLabels();
        RefreshUi();
    }
}

void MainWindow::OnToggleRecord()
{
    const std::uint64_t now = GetTickCount64();
    if (now - last_hotkey_ms_ < HOTKEY_DEBOUNCE_MS) {
        return;
    }
    last_hotkey_ms_ = now;

    if (keybinds_.IsCapturing()) {
        return;
    }

    if (!CheckFile()) {
        return;
    }

    if (state_.is_recording) {
        StopRecording();
        RefreshUi();
        return;
    }

    StopPlaying();
    input_hook_.SetCaptureEnabled(true);
    if (!recorder_.Start(state_.macro_path)) {
        input_hook_.SetCaptureEnabled(false);
        MessageBoxW(
            hwnd_,
            L"Failed to start recording.",
            APP_TITLE,
            MB_OK | MB_ICONERROR);
        return;
    }

    state_.is_recording = true;
    state_.elapsed_ms = 0;
    RefreshUi();
}

void MainWindow::OnTogglePlay()
{
    const std::uint64_t now = GetTickCount64();
    if (now - last_hotkey_ms_ < HOTKEY_DEBOUNCE_MS) {
        return;
    }
    last_hotkey_ms_ = now;

    if (keybinds_.IsCapturing() || state_.is_recording) {
        return;
    }

    if (!CheckFile()) {
        return;
    }

    if (state_.is_playing) {
        StopPlaying();
        state_.elapsed_ms = macro_duration_ms_;
        RefreshUi();
        return;
    }

    if (!player_.Load(state_.macro_path)) {
        MessageBoxW(
            hwnd_,
            L"Failed to load macro file.",
            APP_TITLE,
            MB_OK | MB_ICONERROR);
        return;
    }

    if (!player_.Start(hwnd_, state_.loop_enabled)) {
        MessageBoxW(
            hwnd_,
            L"Macro file is empty.",
            APP_TITLE,
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    state_.is_playing = true;
    state_.is_starting = true;
    state_.elapsed_ms = 3000;
    RefreshUi();
}

void MainWindow::OnPlayFinished()
{
    state_.is_playing = false;
    state_.is_starting = false;
    state_.elapsed_ms = macro_duration_ms_;
    RefreshUi();
}

void MainWindow::OnUiTick()
{
    OnKeybindNotify();

    if (!state_.is_recording && !state_.is_playing) {
        return;
    }

    std::uint64_t elapsed_ms = 0;
    bool starting = false;
    if (state_.is_recording) {
        elapsed_ms = recorder_.ElapsedMs();
    } else {
        starting = player_.IsStarting();
        elapsed_ms = starting
            ? player_.StartupLeftMs()
            : player_.ElapsedMs();
    }

    const std::uint64_t sec = elapsed_ms / 1000;
    if (sec == last_drawn_sec_ && starting == state_.is_starting) {
        return;
    }

    state_.is_starting = starting;
    state_.elapsed_ms = elapsed_ms;
    last_drawn_sec_ = sec;
    InvalidateStatusArea();
}

LRESULT CALLBACK MainWindow::WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam)
{
    MainWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<MainWindow*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandleMessage(msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_COMMAND: {
        switch (LOWORD(wparam)) {
        case IDC_CREATE:
            OnCreateFile();
            break;
        case IDC_OPEN:
            OnOpenFile();
            break;
        case IDC_LOOP:
            OnLoopToggle();
            break;
        case IDC_KEY_RECORD:
            OnKeybindRecord();
            break;
        case IDC_KEY_PLAY:
            OnKeybindPlay();
            break;
        default:
            break;
        }
        return 0;
    }
    case WM_DRAWITEM:
        return TRUE;
    case KeybindManager::WM_KEYBIND:
        OnKeybindNotify();
        return 0;
    case WM_TOGGLE_RECORD:
        OnToggleRecord();
        return 0;
    case WM_TOGGLE_PLAY:
        OnTogglePlay();
        return 0;
    case WM_PLAY_FINISHED:
        OnPlayFinished();
        return 0;
    case WM_TIMER:
        if (wparam == IDT_UI) {
            OnUiTick();
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps = {};
        HDC hdc = BeginPaint(hwnd_, &ps);

        RECT client = {};
        GetClientRect(hwnd_, &client);
        const int width = client.right - client.left;
        const int height = client.bottom - client.top;

        HDC mem_dc = CreateCompatibleDC(hdc);
        HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height);
        HGDIOBJ old_bitmap = SelectObject(mem_dc, bitmap);

        PaintWindow(mem_dc, client);
        BitBlt(hdc, 0, 0, width, height, mem_dc, 0, 0, SRCCOPY);

        SelectObject(mem_dc, old_bitmap);
        DeleteObject(bitmap);
        DeleteDC(mem_dc);
        EndPaint(hwnd_, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd_, IDT_UI);
        StopRecording();
        StopPlaying();
        input_hook_.Uninstall();
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd_, msg, wparam, lparam);
}
