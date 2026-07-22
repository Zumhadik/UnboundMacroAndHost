#include "rdphost/ui/MainWindow.h"

#include "rdphost/WinUtils.h"
#include "rdphost/ui/UiColors.h"

namespace rdphost::ui {
namespace {

constexpr wchar_t kMainClass[] = L"RdpHostMainWindow";

}
MainWindow::MainWindow(std::shared_ptr<AppController> controller)
    : controller_(std::move(controller)) {}

bool MainWindow::Create(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kMainClass;
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;

    if (RegisterClassExW(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    hwnd_ = CreateWindowExW(
        0,
        kMainClass,
        L"RdpHost",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        760,
        520,
        nullptr,
        nullptr,
        instance,
        this);

    if (hwnd_ == nullptr) {
        return false;
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    RefreshDiagnostics();
    return true;
}
int MainWindow::RunMessageLoop() {
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
LRESULT CALLBACK MainWindow::WndProc(HWND hwnd,
                                     UINT msg,
                                     WPARAM wparam,
                                     LPARAM lparam) {
    MainWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self != nullptr) {
        return self->HandleMessage(msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}
LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_SIZE:
            OnSize(LOWORD(lparam), HIWORD(lparam));
            return 0;
        case WM_COMMAND:
            OnCommand(wparam);
            return 0;
        case WM_CTLCOLORSTATIC: {
            const HDC hdc = reinterpret_cast<HDC>(wparam);
            const HWND child = reinterpret_cast<HWND>(lparam);
            if (child == label_action_) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(
                    hdc,
                    last_action_ok_ ? ColorForStatus(StatusLevel::Ok)
                                    : ColorForStatus(StatusLevel::Error));
                return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd_, msg, wparam, lparam);
}
void MainWindow::OnCreate() {
    button_check_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Check",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        12,
        12,
        100,
        28,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdCheck)),
        GetModuleHandleW(nullptr),
        nullptr);

    button_install_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Install / Repair",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        122,
        12,
        140,
        28,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdInstall)),
        GetModuleHandleW(nullptr),
        nullptr);

    button_launch_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Launch RDP",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        272,
        12,
        110,
        28,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdLaunch)),
        GetModuleHandleW(nullptr),
        nullptr);

    button_copy_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Copy Status",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        392,
        12,
        110,
        28,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdCopy)),
        GetModuleHandleW(nullptr),
        nullptr);

    label_action_ = CreateWindowExW(
        0,
        L"STATIC",
        L"Ready.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        12,
        48,
        700,
        22,
        hwnd_,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    status_view_.Create(hwnd_, 12, 78, 720, 380);
}
void MainWindow::OnSize(int width, int height) {
    if (label_action_ != nullptr) {
        MoveWindow(label_action_, 12, 48, width - 24, 22, TRUE);
    }
    if (status_view_.Handle() != nullptr) {
        MoveWindow(status_view_.Handle(), 12, 78, width - 24, height - 90, TRUE);
    }
}
void MainWindow::OnCommand(WPARAM wparam) {
    switch (LOWORD(wparam)) {
        case kIdCheck:
            RefreshDiagnostics();
            break;
        case kIdInstall:
            OnEnsureInstall();
            break;
        case kIdLaunch:
            OnLaunchSession();
            break;
        case kIdCopy:
            OnCopyStatus();
            break;
        default:
            break;
    }
}
void MainWindow::RefreshDiagnostics() {
    const StatusList items = controller_->RunDiagnostics();
    status_view_.SetItems(items);

    bool all_ok = true;
    for (const StatusItem& item : items) {
        if (item.id != L"admin" && item.level == StatusLevel::Error) {
            all_ok = false;
            break;
        }
    }
    SetActionMessage(
        all_ok ? L"Check OK." : L"Check found problems.",
        all_ok);
}
void MainWindow::OnEnsureInstall() {
    const OperationResult result = controller_->EnsureInstalled();
    SetActionMessage(result.message, result.ok);
    RefreshDiagnostics();
}
void MainWindow::OnLaunchSession() {
    SessionOptions options;
    options.host = L"127.0.0.2";
    options.port = 3389;

    const OperationResult result =
        controller_->PrepareAndLaunch(options, 20000);
    SetActionMessage(result.message, result.ok);
    RefreshDiagnostics();
}
void MainWindow::OnCopyStatus() {
    std::wstring text = last_action_message_;
    if (!text.empty()) {
        text += L"\r\n";
    }
    text += status_view_.FormatText();
    if (text.empty()) {
        SetActionMessage(L"Nothing to copy.", false);
        return;
    }

    if (WinUtils::CopyTextToClipboard(hwnd_, text)) {
        SetActionMessage(L"Status copied.", true);
    } else {
        SetActionMessage(L"Copy failed.", false);
    }
}
void MainWindow::SetActionMessage(const std::wstring& message, bool ok) {
    last_action_message_ = message;
    last_action_ok_ = ok;
    if (label_action_ != nullptr) {
        SetWindowTextW(label_action_, message.c_str());
        InvalidateRect(label_action_, nullptr, TRUE);
    }
}
}