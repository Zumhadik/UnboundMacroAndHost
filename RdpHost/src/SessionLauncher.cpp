#include "rdphost/SessionLauncher.h"

#include <windows.h>
#include <shellapi.h>

#include <fstream>
#include <sstream>

namespace rdphost {
namespace {

std::string Narrow(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int needed = WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (needed <= 0) {
        return {};
    }
    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        static_cast<int>(text.size()),
        out.data(),
        needed,
        nullptr,
        nullptr);
    return out;
}

}

SessionLauncher::SessionLauncher(AppPaths paths)
    : paths_(std::move(paths)) {}

OperationResult SessionLauncher::WriteRdpFile(
    const SessionOptions& options) const {
    std::ostringstream stream;
    stream
        << "full address:s:" << Narrow(options.host) << ":" << options.port
        << "\n"
        << "prompt for credentials:i:1\n"
        << "authentication level:i:2\n"
        << "negotiate security layer:i:1\n"
        << "enablecredsspsupport:i:1\n"
        << "desktopwidth:i:" << options.desktopWidth << "\n"
        << "desktopheight:i:" << options.desktopHeight << "\n"
        << "screen mode id:i:" << (options.fullscreen ? 2 : 1) << "\n"
        << "session bpp:i:32\n"
        << "compression:i:1\n"
        << "keyboardhook:i:1\n"
        << "audiomode:i:2\n"
        << "redirectclipboard:i:1\n"
        << "redirectprinters:i:0\n"
        << "redirectcomports:i:0\n"
        << "redirectsmartcards:i:0\n"
        << "displayconnectionbar:i:1\n"
        << "autoreconnection enabled:i:1\n"
        << "alternate shell:s:\n"
        << "shell working directory:s:\n";

    if (!options.username.empty()) {
        stream << "username:s:" << Narrow(options.username) << "\n";
    }

    std::ofstream file(paths_.generatedRdpFile, std::ios::binary | std::ios::trunc);
    if (!file) {
        return {false, L"Cannot write " + paths_.generatedRdpFile};
    }
    file << stream.str();
    if (!file) {
        return {false, L"Failed writing RDP file content"};
    }

    return {true, L"Wrote " + paths_.generatedRdpFile};
}

OperationResult SessionLauncher::LaunchMstsc() const {
    const std::wstring args = L"\"" + paths_.generatedRdpFile + L"\"";
    const HINSTANCE result = ShellExecuteW(
        nullptr,
        L"open",
        L"mstsc.exe",
        args.c_str(),
        nullptr,
        SW_SHOWNORMAL);

    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        return {
            false,
            L"ShellExecute(mstsc) failed, code=" +
                std::to_wstring(reinterpret_cast<INT_PTR>(result))};
    }

    return {true, L"mstsc launched"};
}

OperationResult SessionLauncher::WriteAndLaunch(
    const SessionOptions& options) const {
    OperationResult write = WriteRdpFile(options);
    if (!write.ok) {
        return write;
    }
    return LaunchMstsc();
}

}
