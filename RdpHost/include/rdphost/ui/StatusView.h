#pragma once

#include "rdphost/Types.h"

#include <windows.h>

#include <string>

namespace rdphost::ui {

class StatusView {
public:
    StatusView() = default;
    ~StatusView();

    StatusView(const StatusView&) = delete;
    StatusView& operator=(const StatusView&) = delete;

    bool Create(HWND parent, int x, int y, int width, int height);
    void SetItems(const StatusList& items);
    std::wstring FormatText() const;

    HWND Handle() const;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);
    void Paint(HDC hdc);

    HWND hwnd_ = nullptr;
    StatusList items_;
    static bool class_registered_;
};

}