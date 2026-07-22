#pragma once

#include "rdphost/AppController.h"
#include "rdphost/ui/StatusView.h"

#include <windows.h>

#include <memory>
#include <string>

namespace rdphost::ui {

class MainWindow {
public:
    explicit MainWindow(std::shared_ptr<AppController> controller);

    bool Create(HINSTANCE instance);
    int RunMessageLoop();

private:
    static constexpr int kIdCheck = 1001;
    static constexpr int kIdInstall = 1002;
    static constexpr int kIdLaunch = 1003;
    static constexpr int kIdCopy = 1004;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

    void OnCreate();
    void OnSize(int width, int height);
    void OnCommand(WPARAM wparam);
    void RefreshDiagnostics();
    void OnEnsureInstall();
    void OnLaunchSession();
    void OnCopyStatus();
    void SetActionMessage(const std::wstring& message, bool ok);

    std::shared_ptr<AppController> controller_;
    StatusView status_view_;
    HWND hwnd_ = nullptr;
    HWND button_check_ = nullptr;
    HWND button_install_ = nullptr;
    HWND button_launch_ = nullptr;
    HWND button_copy_ = nullptr;
    HWND label_action_ = nullptr;
    std::wstring last_action_message_;
    bool last_action_ok_ = true;
};

}